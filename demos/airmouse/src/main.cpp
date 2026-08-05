/*
 * OpenMote Gen 3.3 � Air Mouse (No-Magnetometer Version)
 *
 * Turns the OpenMote into an absolute-pointing USB air mouse using 6-DOF sensor
 * fusion (accel+gyro). Pitch and roll are absolute via gravity, while yaw
 * (heading) is integrated from the gyro.
 *
 * Controls
 *   A      left click / hold = drag
 *   B      right click
 *   HOME   recenter � point at the middle of the screen and tap to re-zero aim
 *   +/-    sensitivity (smaller / larger field of view)
 *   1      recalibrate gyro (hold still; rejects if moved)
 *   2      scroll wheel down
 *   POWER  pause / resume pointing
 *
 * Serial console (USB CDC, 115200) live-tuning commands:
 *   c recenter   r reset filter   g gyro-bias cal
 *   + / -  field of view          d toggle debug stream
 *   x / y  invert X / Y axis      s swap yaw/pitch axes
 */

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <math.h>

#include "USB.h"
#include "pin_definitions.h"
#include "MahonyAHRS.h"
#include "AbsoluteMouse.h"

// ============================================================
// Tunable defaults
// ============================================================
static float  FOV_X_DEG   = 60.0f;
static float  FOV_Y_DEG   = 40.0f;
static float  MAHONY_KP   = 1.0f;
static float  MAHONY_KI   = 0.10f;
static float  POINT_SMOOTH = 0.70f;
static float  ACCEL_GATE_G = 0.35f;

static bool   INVERT_X    = false;
static bool   INVERT_Y    = true;
static bool   SWAP_AXES   = false;

// ============================================================
// Globals
// ============================================================
AbsoluteMouse Mouse;
MahonyAHRS    ahrs(MAHONY_KP, MAHONY_KI);
Preferences   prefs;

struct Calib {
    float gbx = 0, gby = 0, gbz = 0;
} cal;

static float  azRef = 0, elRef = 0;
static float  smX = 0.5f, smY = 0.5f;
static bool   armed = false;
static bool   debugStream = false;
static uint32_t lastUs = 0;

// ============================================================
// I2C register helpers
// ============================================================
static bool i2cReadRegs(uint8_t addr, uint8_t reg, uint8_t *buf, size_t n) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)addr, (int)n) != (int)n) return false;
    for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
    return true;
}
static bool i2cWriteReg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

// ============================================================
// Sensor bring-up
// ============================================================
static bool imuInit() {
    uint8_t who = 0;
    if (!i2cReadRegs(I2C_ADDR_IMU, IMU_REG_WHOAMI, &who, 1) || who != 0x6A) return false;
    i2cWriteReg(I2C_ADDR_IMU, IMU_REG_CTRL3_C, 0x44);
    i2cWriteReg(I2C_ADDR_IMU, IMU_REG_CTRL1_XL, 0x60);
    i2cWriteReg(I2C_ADDR_IMU, IMU_REG_CTRL2_G,  0x68);
    delay(20);
    return true;
}

static bool imuRead(float &ax, float &ay, float &az,
                    float &gx, float &gy, float &gz) {
    uint8_t d[12];
    if (!i2cReadRegs(I2C_ADDR_IMU, IMU_REG_OUTX_L_G, d, 12)) return false;
    int16_t rgx = (int16_t)(d[0] | (d[1] << 8));
    int16_t rgy = (int16_t)(d[2] | (d[3] << 8));
    int16_t rgz = (int16_t)(d[4] | (d[5] << 8));
    int16_t rax = (int16_t)(d[6] | (d[7] << 8));
    int16_t ray = (int16_t)(d[8] | (d[9] << 8));
    int16_t raz = (int16_t)(d[10] | (d[11] << 8));

    ax = rax / IMU_ACC_LSB_PER_G;
    ay = ray / IMU_ACC_LSB_PER_G;
    az = raz / IMU_ACC_LSB_PER_G;

    const float D2R = (float)M_PI / 180.0f;
    gx = (rgx * IMU_GYR_DPS_PER_LSB - cal.gbx) * D2R;
    gy = (rgy * IMU_GYR_DPS_PER_LSB - cal.gby) * D2R;
    gz = (rgz * IMU_GYR_DPS_PER_LSB - cal.gbz) * D2R;
    return true;
}

// ============================================================
// Calibration
// ============================================================
static void saveCalib() {
    prefs.begin("airmouse", false);
    prefs.putBytes("calib", &cal, sizeof(cal));
    prefs.end();
}
static void loadCalib() {
    prefs.begin("airmouse", true);
    if (prefs.getBytesLength("calib") == sizeof(cal))
        prefs.getBytes("calib", &cal, sizeof(cal));
    prefs.end();
}

// Reject a calibration if the per-axis gyro jitter (RMS / standard deviation)
// exceeds this — that means the controller was being moved, so the average would
// bake real rotation into the "bias" and cause permanent drift. RMS is used
// instead of peak-to-peak because a single noisy I2C sample blows up p2p and
// would reject an otherwise perfectly still board.
static const float GYRO_STILL_RMS_DPS = 1.0f;

// Returns true if a new bias was accepted, false if rejected (motion / no data).
// On reject the previous bias is left untouched.
static bool calibrateGyro(uint16_t samples = 400) {
    Serial.println("[gyro] hold the controller STILL...");
    double sx = 0, sy = 0, sz = 0;          // sums (for the mean / bias)
    double sx2 = 0, sy2 = 0, sz2 = 0;       // sums of squares (for the variance)
    uint16_t got = 0;
    float ax, ay, az, gx, gy, gz;
    float saveGbx = cal.gbx, saveGby = cal.gby, saveGbz = cal.gbz;
    cal.gbx = cal.gby = cal.gbz = 0;
    const float R2D = 180.0f / (float)M_PI;
    for (uint16_t i = 0; i < samples; i++) {
        if (imuRead(ax, ay, az, gx, gy, gz)) {
            float dx = gx * R2D, dy = gy * R2D, dz = gz * R2D;
            sx  += dx;            sy  += dy;            sz  += dz;
            sx2 += (double)dx*dx; sy2 += (double)dy*dy; sz2 += (double)dz*dz;
            got++;
        }
        delay(3);
    }
    if (got <= 50) {
        cal.gbx = saveGbx; cal.gby = saveGby; cal.gbz = saveGbz;
        Serial.println("[gyro] FAILED (no samples)");
        return false;
    }
    float mx = sx / got, my = sy / got, mz = sz / got;
    // var = E[x^2] - mean^2, clamped to 0 against float round-off.
    float vx = fmaxf(0.0f, (float)(sx2 / got) - mx * mx);
    float vy = fmaxf(0.0f, (float)(sy2 / got) - my * my);
    float vz = fmaxf(0.0f, (float)(sz2 / got) - mz * mz);
    float rms = sqrtf(fmaxf(vx, fmaxf(vy, vz)));
    if (rms > GYRO_STILL_RMS_DPS) {
        cal.gbx = saveGbx; cal.gby = saveGby; cal.gbz = saveGbz;
        Serial.printf("[gyro] REJECTED — jitter rms=%.2f dps (limit %.2f); kept old bias\n",
                      rms, GYRO_STILL_RMS_DPS);
        return false;
    }
    cal.gbx = mx; cal.gby = my; cal.gbz = mz;
    Serial.printf("[gyro] bias dps  x=%.3f y=%.3f z=%.3f  (rms=%.2f)\n",
                  cal.gbx, cal.gby, cal.gbz, rms);
    saveCalib();
    return true;
}

// ============================================================
// LED feedback (4 player LEDs)
// ============================================================
static const uint8_t LEDS[4] = { LED_1, LED_2, LED_3, LED_4 };

static void ledsAll(bool on) {
    for (uint8_t i = 0; i < 4; i++) digitalWrite(LEDS[i], on ? HIGH : LOW);
}

// Restore the steady at-rest LED state (LED_1 = armed indicator, rest off).
static void ledsRestore() {
    digitalWrite(LED_1, armed ? HIGH : LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW);
    digitalWrite(LED_4, LOW);
}

// Rejected calibration: flash all four LEDs together.
static void ledFlashAll(uint8_t times) {
    for (uint8_t t = 0; t < times; t++) {
        ledsAll(true);  delay(120);
        ledsAll(false); delay(120);
    }
}

// Accepted calibration: one LED_1 -> LED_4 chase.
static void ledChaseOnce() {
    for (uint8_t i = 0; i < 4; i++) {
        digitalWrite(LEDS[i], HIGH);
        delay(90);
        digitalWrite(LEDS[i], LOW);
    }
}

// ============================================================
// Pointing
// ============================================================
static float wrapPi(float a) {
    while (a >  (float)M_PI) a -= 2.0f * (float)M_PI;
    while (a < -(float)M_PI) a += 2.0f * (float)M_PI;
    return a;
}

static void aimAngles(float &az, float &el) {
    float px, py, pz;
    ahrs.bodyToWorld(AIM_AXIS_X, AIM_AXIS_Y, AIM_AXIS_Z, px, py, pz);
    float n = sqrtf(px * px + py * py + pz * pz);
    if (n > 1e-6f) { px /= n; py /= n; pz /= n; }
    az = atan2f(px, py);
    el = asinf(constrain(pz, -1.0f, 1.0f));
}

static void recenter() {
    aimAngles(azRef, elRef);
    smX = smY = 0.5f;
    Serial.println("[aim] recentered");
}

// Recalibrate the gyro bias with visible pass/fail feedback. Pointing is paused
// while sampling so the cursor can't run off; on success the aim is re-zeroed.
static void recalibrateWithFeedback() {
    bool wasArmed = armed;
    armed = false;                       // freeze the pointer during sampling
    bool ok = calibrateGyro();
    if (ok) {
        ledChaseOnce();                  // accepted: chase animation
        if (wasArmed) recenter();        // drop the drift we accumulated, re-zero
    } else {
        ledFlashAll(4);                  // rejected: flash all LEDs, bias unchanged
    }
    armed = wasArmed;
    ledsRestore();
    lastUs = micros();                   // discard the long dt from the blocking cal
}

static void updatePointer() {
    float az, el;
    aimAngles(az, el);
    float dAz = wrapPi(az - azRef);
    float dEl = el - elRef;

    float fovX = radians(FOV_X_DEG);
    float fovY = radians(FOV_Y_DEG);

    float aX = SWAP_AXES ? dEl : dAz;
    float aY = SWAP_AXES ? dAz : dEl;
    float spanX = SWAP_AXES ? fovY : fovX;
    float spanY = SWAP_AXES ? fovX : fovY;

    float nx = 0.5f + (INVERT_X ? -1.0f : 1.0f) * (aX / spanX);
    float ny = 0.5f + (INVERT_Y ? -1.0f : 1.0f) * (aY / spanY);

    nx = constrain(nx, 0.0f, 1.0f);
    ny = constrain(ny, 0.0f, 1.0f);

    smX += (nx - smX) * (1.0f - POINT_SMOOTH);
    smY += (ny - smY) * (1.0f - POINT_SMOOTH);

    Mouse.moveTo((uint16_t)(smX * ABS_MOUSE_LOGICAL_MAX),
                 (uint16_t)(smY * ABS_MOUSE_LOGICAL_MAX));
}

// ============================================================
// Buttons
// ============================================================
struct Btn { uint8_t pin; bool last; uint32_t tEdge; };
static Btn btnA{BTN_A, true, 0}, btnB{BTN_B, true, 0};
static Btn btn1{BTN_1, true, 0}, btn2{BTN_2, true, 0};

static int edge(Btn &b) {
    bool now = digitalRead(b.pin);
    if (now != b.last && (millis() - b.tEdge) > 15) {
        b.last = now; b.tEdge = millis();
        return now ? -1 : +1;
    }
    return 0;
}

struct TapBtn { uint8_t pin; bool down; uint32_t tDown; bool holdFired; };
static TapBtn tbHome{BTN_HOME, false, 0, false}, tbPower{BTN_POWER, false, 0, false};
static TapBtn tbPlus{BTN_PLUS, false, 0, false}, tbMinus{BTN_MINUS, false, 0, false};

enum BtnEv { EV_NONE, EV_TAP, EV_HOLD };
static const uint32_t HOLD_MS = 800;

static BtnEv poll(TapBtn &b) {
    bool pressed = (digitalRead(b.pin) == LOW);
    if (pressed && !b.down) { b.down = true; b.tDown = millis(); b.holdFired = false; }
    else if (!pressed && b.down) {
        b.down = false;
        if (!b.holdFired && millis() - b.tDown < HOLD_MS) return EV_TAP;
    }
    if (pressed && b.down && !b.holdFired && millis() - b.tDown >= HOLD_MS) {
        b.holdFired = true; return EV_HOLD;
    }
    return EV_NONE;
}

static void feedback(uint8_t pulses) {
    bool a = (digitalRead(BTN_A) == LOW);
    for (uint8_t i = 0; i < pulses; i++) {
        digitalWrite(PIN_RUMBLE, HIGH); digitalWrite(LED_2, HIGH); delay(45);
        digitalWrite(PIN_RUMBLE, LOW);  digitalWrite(LED_2, LOW);  delay(90);
    }
    digitalWrite(LED_2, (armed && a) ? HIGH : LOW);
}

static void adjustFov(float dx, float dy) {
    FOV_X_DEG = constrain(FOV_X_DEG + dx, 15.0f, 150.0f);
    FOV_Y_DEG = constrain(FOV_Y_DEG + dy, 10.0f, 120.0f);
    Serial.printf("[fov] x=%.0f y=%.0f deg\n", FOV_X_DEG, FOV_Y_DEG);
}

static void setPaused(bool paused) {
    if (paused == !armed) return;
    armed = !paused;
    if (paused) { Mouse.release(ABS_MOUSE_LEFT); Mouse.release(ABS_MOUSE_RIGHT); }
    else        { recenter(); }
    digitalWrite(LED_1, armed ? HIGH : LOW);
    Serial.printf("[point] %s\n", armed ? "ACTIVE" : "PAUSED");
    feedback(armed ? 2 : 1);
}

static void handleButtons() {
    int e;
    if ((e = edge(btnA))) {
        if (e > 0 && armed) { Mouse.press(ABS_MOUSE_LEFT);  digitalWrite(LED_2, HIGH); }
        else                { Mouse.release(ABS_MOUSE_LEFT); digitalWrite(LED_2, LOW);  }
    }
    if ((e = edge(btnB))) {
        if (e > 0 && armed) Mouse.press(ABS_MOUSE_RIGHT); else Mouse.release(ABS_MOUSE_RIGHT);
    }
    if (edge(btn1) > 0) recalibrateWithFeedback();   // BTN_1 = gyro recalibrate
    if (edge(btn2) > 0 && armed) Mouse.scroll(-1);

    if (poll(tbHome) == EV_TAP)  { recenter(); feedback(1); }
    if (poll(tbPower) == EV_TAP) { setPaused(armed); }
    if (poll(tbPlus)  == EV_TAP) { adjustFov(-5.0f, -3.0f); feedback(1); }
    if (poll(tbMinus) == EV_TAP) { adjustFov(+5.0f, +3.0f); feedback(1); }
}

// ============================================================
// Serial console
// ============================================================
static void handleSerial() {
    while (Serial.available()) {
        char c = Serial.read();
        switch (c) {
            case 'c': recenter(); break;
            case 'r': ahrs.reset(); Serial.println("[ahrs] reset"); break;
            case 'g': recalibrateWithFeedback(); break;
            case '+': adjustFov(-5.0f, -3.0f); break;
            case '-': adjustFov(+5.0f, +3.0f); break;
            case 'x': INVERT_X = !INVERT_X; Serial.printf("[axis] invertX=%d\n", INVERT_X); break;
            case 'y': INVERT_Y = !INVERT_Y; Serial.printf("[axis] invertY=%d\n", INVERT_Y); break;
            case 's': SWAP_AXES = !SWAP_AXES; Serial.printf("[axis] swap=%d\n", SWAP_AXES); break;
            case '{': POINT_SMOOTH = constrain(POINT_SMOOTH - 0.05f, 0.0f, 0.98f);
                      Serial.printf("[smooth] %.2f\n", POINT_SMOOTH); break;
            case '}': POINT_SMOOTH = constrain(POINT_SMOOTH + 0.05f, 0.0f, 0.98f);
                      Serial.printf("[smooth] %.2f\n", POINT_SMOOTH); break;
            case '[': ACCEL_GATE_G = constrain(ACCEL_GATE_G - 0.02f, 0.02f, 0.5f);
                      Serial.printf("[gate] %.2f g\n", ACCEL_GATE_G); break;
            case ']': ACCEL_GATE_G = constrain(ACCEL_GATE_G + 0.02f, 0.02f, 0.5f);
                      Serial.printf("[gate] %.2f g\n", ACCEL_GATE_G); break;
            case 'd': debugStream = !debugStream; break;
            default: break;
        }
    }
}

// ============================================================
// Setup / loop
// ============================================================
void setup() {
    Serial.begin(115200);

    pinMode(LED_1, OUTPUT); pinMode(LED_2, OUTPUT);
    pinMode(LED_3, OUTPUT); pinMode(LED_4, OUTPUT);
    digitalWrite(LED_1, LOW); digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW); digitalWrite(LED_4, LOW);
    pinMode(PIN_RUMBLE, OUTPUT); digitalWrite(PIN_RUMBLE, LOW);
    pinMode(BTN_A, INPUT_PULLUP);     pinMode(BTN_B, INPUT_PULLUP);
    pinMode(BTN_HOME, INPUT_PULLUP);  pinMode(BTN_PLUS, INPUT_PULLUP);
    pinMode(BTN_MINUS, INPUT_PULLUP); pinMode(BTN_1, INPUT_PULLUP);
    pinMode(BTN_2, INPUT_PULLUP);     pinMode(BTN_POWER, INPUT_PULLUP);

    Mouse.begin();
    USB.begin();

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 1500) delay(10);
    Serial.println("\nOpenMote Gen 3.3 Air Mouse (No-Mag)");

    if (!imuInit()) Serial.println("IMU FAIL");
    else Serial.println("IMU OK");

    loadCalib();
    ahrs.setGains(MAHONY_KP, MAHONY_KI);
    calibrateGyro();

    Serial.println("[init] converging fusion...");
    float ax, ay, az, gx, gy, gz;
    uint32_t cs = millis(); lastUs = micros();
    while (millis() - cs < 1500) {
        if (imuRead(ax, ay, az, gx, gy, gz)) {
            uint32_t now = micros();
            float dt = (now - lastUs) * 1e-6f; lastUs = now;
            if (dt > 0 && dt < 0.2f) ahrs.update(gx, gy, gz, ax, ay, az, 0, 0, 0, dt);
        }
        delay(3);
    }
    recenter();
    armed = true;
    digitalWrite(LED_1, HIGH);
    feedback(2);
    lastUs = micros();
}

void loop() {
    float ax, ay, az, gx, gy, gz;

    if (imuRead(ax, ay, az, gx, gy, gz)) {
        uint32_t now = micros();
        float dt = (now - lastUs) * 1e-6f;
        lastUs = now;
        if (dt > 0 && dt < 0.2f) {
            float amag = sqrtf(ax * ax + ay * ay + az * az);
            if (fabsf(amag - 1.0f) > ACCEL_GATE_G) { ax = ay = az = 0; }
            ahrs.update(gx, gy, gz, ax, ay, az, 0, 0, 0, dt);
            if (armed) updatePointer();
        }
    }

    handleButtons();
    handleSerial();

    if (debugStream) {
        static uint32_t tdbg = 0;
        if (millis() - tdbg > 100) {
            tdbg = millis();
            float az, el; aimAngles(az, el);
            Serial.printf("aim az=%.1f el=%.1f  cur=(%.2f,%.2f)\n",
                          degrees(az), degrees(el), smX, smY);
        }
    }
    delayMicroseconds(500);
}
