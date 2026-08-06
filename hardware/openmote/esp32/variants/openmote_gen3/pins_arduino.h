#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>
#include "soc/soc_caps.h"

// ---------------------------------------------------------------------------
// OpenMote Gen 3.3 — ESP32-S3-WROOM-1-N16R8
//
// These are the same numbers as OpenMotePins.h in the OpenMote library. This
// file exists so that the standard Arduino identifiers (LED_BUILTIN, SDA, SCL)
// mean the right thing on this board; the library remains the reference for the
// OpenMote-specific names, and you do not need this file to use it.
// ---------------------------------------------------------------------------

// Native USB (USB-Serial/JTAG). Every ESP32-S3 reports this pair, so the IDE
// cannot tell an OpenMote from any other S3 board and will show the port as
// "ESP32 Family Device". That is expected.
#define USB_VID 0x303a
#define USB_PID 0x1001

// -- Standard Arduino names -------------------------------------------------

// Player LED 1. Plain LED, not addressable, so no RGB_BUILTIN here.
static const uint8_t LED_BUILTIN = 10;
#define BUILTIN_LED LED_BUILTIN
#define LED_BUILTIN LED_BUILTIN

static const uint8_t SDA = 16;
static const uint8_t SCL = 15;

// WARNING: UART0's default pins are wired to D-pad Up and Down on this board.
// They are named here for completeness, but driving a serial port on them fights
// the buttons: pressing Up or Down injects characters, and logging drives the
// button lines. Use USB CDC for the console.
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// -- Player LEDs (active high) ----------------------------------------------
static const uint8_t LED1 = 10;
static const uint8_t LED2 = 9;
static const uint8_t LED3 = 46;   // strapping pin (ROM message) — runtime use only
static const uint8_t LED4 = 3;    // strapping pin (JTAG source) — runtime use only

// -- Buttons (active low, need INPUT_PULLUP) --------------------------------
static const uint8_t BTN_UP_PIN    = 43;
static const uint8_t BTN_DOWN_PIN  = 44;
static const uint8_t BTN_LEFT_PIN  = 40;
static const uint8_t BTN_RIGHT_PIN = 2;
static const uint8_t BTN_A_PIN     = 38;
static const uint8_t BTN_B_PIN     = 42;
static const uint8_t BTN_1_PIN     = 18;
static const uint8_t BTN_2_PIN     = 8;
static const uint8_t BTN_PLUS_PIN  = 4;
static const uint8_t BTN_MINUS_PIN = 13;
static const uint8_t BTN_HOME_PIN  = 17;
static const uint8_t BTN_POWER_PIN = 0;    // also the BOOT strapping pin

// -- Audio ------------------------------------------------------------------
static const uint8_t I2S_LRCLK = 5;
static const uint8_t I2S_BCLK  = 6;
static const uint8_t I2S_DIN   = 7;
static const uint8_t AMP_SD    = 21;   // analog mode pin, not a plain enable
static const uint8_t MIC_DATA  = 11;
static const uint8_t MIC_CLK   = 12;

// -- Infrared ---------------------------------------------------------------
static const uint8_t IR_RX = 1;        // IRM-56384 receiver, ACTIVE LOW
static const uint8_t IR_TX = 41;       // IR LED via MMBT2222A

// -- Misc -------------------------------------------------------------------
static const uint8_t RUMBLE    = 39;   // SI2302 gate; buzzes in download mode
static const uint8_t BATT_ADC  = 14;   // ADC2 — unusable while Wi-Fi is on
static const uint8_t IMU_INT1  = 48;
static const uint8_t IMU_INT2  = 47;
static const uint8_t BREAKOUT1 = 45;   // strapping pin (VDD_SPI)

// GPIO 26-32 are the SPI flash and 33-37 the Octal PSRAM on the N16R8 module.
// Touching them hangs the board, so they are deliberately not named here.

#endif /* Pins_Arduino_h */
