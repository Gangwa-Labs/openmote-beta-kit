#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

// ============================================================
// OpenMote Gen 3.3 � Air Mouse (No-Magnetometer Version)
// GPIO assignments sourced from ../HARDWARE_REFERENCE.md
// MCU: ESP32-S3-WROOM-1-N16R8 (16 MB flash, 8 MB Octal PSRAM)
// ============================================================

// ---- I2C bus (IMU shares SCL/SDA) ----
#define PIN_SCL          15
#define PIN_SDA          16

// ---- Sensor interrupts (IMU data-ready, optional) ----
#define PIN_IMU_INT1     48
#define PIN_IMU_INT2     47

// ---- Player / status LEDs (active high) ----
#define LED_1            10   // armed / pointing-active indicator
#define LED_2             9   // left button held (drag) indicator
#define LED_3            46   // status LED (D3)  -- GPIO46 is a ROM-msg strap; drive at runtime only
#define LED_4             3   // status LED (D4)  -- GPIO3 is a JTAG-src strap; drive at runtime only

// ---- Buttons (active low, INPUT_PULLUP; pressed = LOW) ----
#define BTN_A            38   // LEFT click + click-and-drag
#define BTN_B            42   // RIGHT click
#define BTN_HOME         17   // Recenter
#define BTN_PLUS          4   // Increase sensitivity (smaller FOV)
#define BTN_MINUS        13   // Decrease sensitivity (larger FOV)
#define BTN_1            18   // Scroll wheel up
#define BTN_2             8   // Scroll wheel down
#define BTN_POWER         0   // Pause / resume pointing

// ---- Haptics: rumble motor ----
#define PIN_RUMBLE       39

// ---- Sensor axis frame ----
#define AIM_AXIS_X   0.0f
#define AIM_AXIS_Y   1.0f
#define AIM_AXIS_Z   0.0f

// ---- I2C device addresses ----
#define I2C_ADDR_IMU     0x6A

// ---- LSM6DS3TR-C registers ----
#define IMU_REG_WHOAMI    0x0F
#define IMU_REG_CTRL1_XL  0x10
#define IMU_REG_CTRL2_G   0x11
#define IMU_REG_CTRL3_C   0x12
#define IMU_REG_OUTX_L_G  0x22
#define IMU_REG_OUTX_L_XL 0x28

#define IMU_ACC_LSB_PER_G   16384.0f
#define IMU_GYR_DPS_PER_LSB 0.035f

#endif // PIN_DEFINITIONS_H
