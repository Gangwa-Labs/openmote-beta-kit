#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

// ============================================================
// OpenMote Gen 3.3 — USB Headset firmware
// All GPIO assignments sourced from HARDWARE_REFERENCE.md
// ESP32-S3-WROOM-1-N16R8 (16 MB flash, 8 MB Octal PSRAM)
//
// ** PORTED FROM GEN 3.2 ** — pin differences vs 3.2:
//   GPIO21: was IO_Breakout1, now AMP_SD (MAX98357A SD_MODE amp enable)
//   GPIO45: was IO_Breakout2, now the only IO_Breakout1
//   MMC5603NJ magnetometer added on the shared I2C bus (0x30)
// Everything this firmware actually touches (PDM mic, LEDs, rumble) is
// unchanged. See the AMP_SD note below for why it still matters here.
// ============================================================

// I2C (IMU / magnetometer / J15 expansion)
#define I2C_SCL          15
#define I2C_SDA          16
#define LSM6DS3_I2C_ADDR 0x6A
#define MMC5603_I2C_ADDR 0x30  // magnetometer — NEW on Gen 3.3

// Audio TX — MAX98357A I2S amp (mono speaker)
#define I2S_LRCLK        5
#define I2S_BCLK         6
#define I2S_DIN          7
// SD_MODE amp enable (NEW on Gen 3.3): HIGH = amp on ((L+R)/2 mono),
// LOW = shutdown. Reaches the SD pin via R17 (680 k) + the JP1 3-pole jumper,
// so it is a weak analog bias — verify the SD voltage on a real board.
// This firmware is mic-only, so AMP_SD is driven LOW to keep the amp muted
// (on Gen 3.2 SD was hard-wired on and the amp idled powered).
#define AMP_SD           21

// Audio RX — LMD4030T261-OA1 PDM MEMS mic
// L/R pin is strapped LEFT on the OpenMote PCB → data is valid
// while CLK is HIGH; configure ESP-IDF PDM slot mode accordingly.
#define MIC_DATA         11
#define MIC_CLK          12

// Haptics
#define RMBL             39

// Player / status LEDs
#define LED_1            10
#define LED_2            9
#define LED_3            46
#define LED_4            3

// Infrared
#define IRM              1
#define IR40             41

// System / breakouts
#define POWER_BUTT       0
#define IO_BREAKOUT1     45  // was IO_BREAKOUT2 on Gen 3.2 — VDD_SPI strap
#define BATT_LEVEL       14

// IMU interrupts
#define IMU_INT1         48
#define IMU_INT2         47

// D-Pad
#define BTN_UP           43
#define BTN_DOWN         44
#define BTN_LEFT         40
#define BTN_RIGHT        2

// Action buttons
#define BTN_A            38
#define BTN_B            42
#define BTN_1            18
#define BTN_2            8

// Menu buttons
#define BTN_PLUS         4
#define BTN_MINUS        13
#define BTN_HOME         17

// Reserved on the N16R8 module — do not assign:
//   GPIO26-32  SPI flash
//   GPIO33-37  Octal PSRAM

#endif // PIN_DEFINITIONS_H
