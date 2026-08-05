#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

// ========================================
// OpenMote Gen 3.3 Board Pin Definitions
// BLE TV Remote (IR power)
// ESP32-S3-WROOM-1-N16R8
// ========================================

// Haptics
#define RMBL            39  // Rumble Motor Control

// LEDs (active high)
#define LED_1           10  // Player LED 1
#define LED_2           9   // Player LED 2  - BLE status
#define LED_3           46  // Player LED 3  - strapping pin
#define LED_4           3   // Player LED 4  - strapping pin

// Infrared
#define IR40            41  // IR LED Transmitter (38kHz NEC)
#define IR_RECV         1   // IR Receiver input (IRM-56384, active low)

// Buttons - D-Pad
#define UP_BUTT         43  // D-Pad Up      - default U0TXD
#define DOWN_BUTT       44  // D-Pad Down    - default U0RXD
#define LEFT_BUTT       40  // D-Pad Left
#define RIGHT_BUTT      2   // D-Pad Right

// Buttons - Action
#define A_BUTT          38  // A Button
#define B_BUTT          42  // B Button (Trigger)
#define BUTTON_1        18  // 1 Button - unused
#define BUTTON_2        8   // 2 Button - unused

// Buttons - Menu
#define PLUS_BUTT       4   // Plus (+) Button
#define MINUS_BUTT      13  // Minus (-) Button
#define HOME_BUTT       17  // Home Button
#define POWER_BUTT      0   // Power / Boot Button - strapping pin, sends the IR code

// LED array for easy iteration
const int LED_PINS[] = {LED_1, LED_2, LED_3, LED_4};
#define NUM_LEDS 4

#endif // PIN_DEFINITIONS_H
