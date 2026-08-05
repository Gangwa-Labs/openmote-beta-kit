/*
 * 04 — Motion and compass
 *
 * Streams the accelerometer, gyroscope and magnetometer, plus derived pitch,
 * roll and heading.
 *
 *   A     cycle what gets printed
 *   B     run the magnetometer calibration routine
 *
 * About the heading: a compass is only as good as its calibration. The speaker
 * magnet, the battery and any steel near the board all bend the field. Run the
 * calibration (B) before believing the number, and redo it if you change the
 * enclosure.
 */
#include <OpenMote.h>

OpenMote mote;

enum Mode { MODE_ACCEL, MODE_GYRO, MODE_MAG, MODE_ANGLES, MODE_COUNT };
Mode mode = MODE_ACCEL;

const char *modeNames[] = {"accelerometer", "gyroscope", "magnetometer", "angles"};

void calibrateMag() {
    Serial.println("\nMagnetometer calibration.");
    Serial.println("Slowly rotate the OpenMote through every orientation -");
    Serial.println("think of drawing a figure-8 in the air. 15 seconds...");

    float minX =  1e9, minY =  1e9, minZ =  1e9;
    float maxX = -1e9, maxY = -1e9, maxZ = -1e9;

    uint32_t start = millis();
    while (millis() - start < 15000) {
        MagData m = mote.readMagRaw();          // calibrate in the chip's frame
        if (m.valid) {
            if (m.mx < minX) minX = m.mx;  if (m.mx > maxX) maxX = m.mx;
            if (m.my < minY) minY = m.my;  if (m.my > maxY) maxY = m.my;
            if (m.mz < minZ) minZ = m.mz;  if (m.mz > maxZ) maxZ = m.mz;
        }
        // Sweep the LEDs so it is obvious the routine is running.
        mote.ledBar(1 + ((millis() - start) / 1000) % 4);
        delay(20);
    }
    mote.allLEDsOff();

    // The hard-iron offset is the centre of the sphere the readings trace out.
    float offX = (minX + maxX) / 2.0f;
    float offY = (minY + maxY) / 2.0f;
    float offZ = (minZ + maxZ) / 2.0f;
    mote.setMagCalibration(offX, offY, offZ);

    Serial.printf("Done. Offsets: %.2f, %.2f, %.2f uT\n", offX, offY, offZ);
    Serial.printf("Field span:    %.1f, %.1f, %.1f uT\n",
                  maxX - minX, maxY - minY, maxZ - minZ);
    Serial.println("Paste this into your setup() to skip calibrating next time:");
    Serial.printf("    mote.setMagCalibration(%.2ff, %.2ff, %.2ff);\n\n",
                  offX, offY, offZ);
    mote.rumble(150);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    mote.begin();

    if (!mote.imuAvailable()) Serial.println("IMU missing - check the I2C bus.");
    if (!mote.magAvailable()) Serial.println("Magnetometer missing.");

    // Defaults are +/-2 g and 1000 deg/s. Widen the accel range if you are
    // logging impacts, or narrow the gyro range for finer slow rotation.
    mote.setAccelRange(ACCEL_4G);
    mote.setGyroRange(GYRO_1000DPS);

    Serial.println("A = change display, B = calibrate compass.");
}

void loop() {
    mote.update();

    if (mote.buttonPressed(BTN_A)) {
        mode = (Mode)((mode + 1) % MODE_COUNT);
        Serial.printf("\n--- %s ---\n", modeNames[mode]);
    }
    if (mote.buttonPressed(BTN_B)) calibrateMag();

    static uint32_t last = 0;
    if (millis() - last >= 100) {
        last = millis();

        switch (mode) {
            case MODE_ACCEL: {
                IMUData d = mote.readIMU();
                if (d.valid) Serial.printf("accel  %+6.2f %+6.2f %+6.2f g\n",
                                           d.ax, d.ay, d.az);
                break;
            }
            case MODE_GYRO: {
                IMUData d = mote.readIMU();
                if (d.valid) Serial.printf("gyro   %+7.1f %+7.1f %+7.1f deg/s\n",
                                           d.gx, d.gy, d.gz);
                break;
            }
            case MODE_MAG: {
                MagData m = mote.readMag();
                if (m.valid) Serial.printf("mag    %+7.1f %+7.1f %+7.1f uT\n",
                                           m.mx, m.my, m.mz);
                break;
            }
            case MODE_ANGLES: {
                Serial.printf("pitch %+6.1f  roll %+6.1f  heading %5.1f  %.1f C\n",
                              mote.pitch(), mote.roll(),
                              mote.headingDegrees(), mote.readTemperature());
                break;
            }
            default: break;
        }
    }
}
