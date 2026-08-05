/*
 * OpenMote Gen 3.3 — PlatformIO starter
 *
 * Press A to light LED 1 and buzz. Hold Home for a second to run an I2C scan.
 */
#include <Arduino.h>
#include <OpenMote.h>

OpenMote mote;

void setup() {
    Serial.begin(115200);
    delay(500);                     // give the USB CDC port a moment to enumerate

    Serial.println("\nOpenMote Gen 3.3 starting up");

    if (!mote.begin()) {
        Serial.println("Warning: the IMU did not come up.");
    }
    Serial.printf("  IMU:          %s\n", mote.imuAvailable() ? "ok" : "MISSING");
    Serial.printf("  Battery:      %.2f V (%u%%)\n",
                  mote.batteryVolts(), mote.batteryPercent());

    mote.ledBar(4);
    delay(300);
    mote.allLEDsOff();
}

void loop() {
    mote.update();

    if (mote.buttonPressed(BTN_A)) {
        mote.setLED(1, true);
        mote.rumble(150);
        Serial.println("A pressed");
    }
    if (mote.buttonReleased(BTN_A)) {
        mote.setLED(1, false);
    }

    static bool scanned = false;
    if (mote.buttonHeldFor(BTN_HOME, 1000)) {
        if (!scanned) { mote.i2cScan(); scanned = true; }
    } else if (!mote.buttonDown(BTN_HOME)) {
        scanned = false;
    }

    delay(5);
}
