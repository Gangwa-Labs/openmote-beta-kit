/*
 * OpenMote Gen 3.3 - BLE TV Remote
 *
 * The TV half of the `light+tv` project with the WiZ light / WiFi control
 * removed: BLE HID keyboard for Fire TV, plus IR for TV power.
 *
 * Button Mappings:
 * - Power Button  -> replay the stored TV power IR code (NEC 0xB9467D02, 38kHz)
 * - A Button      -> Select/OK (Enter key)          [BLE]
 * - B Button      -> Back (Escape key)              [BLE]
 * - Home Button   -> Home Menu (consumer control)   [BLE]
 * - D-Pad Arrows  -> Navigation (Arrow keys)        [BLE]
 * - Plus Button   -> Volume Up (consumer control)   [BLE]
 * - Minus Button  -> Volume Down (consumer control) [BLE]
 * - Buttons 1 / 2 -> unused
 *
 * LED 2 slow-blinks while BLE is disconnected.
 *
 * Hardware: OpenMote Gen 3.3 (ESP32-S3-WROOM-1-N16R8)
 * Libraries:
 * - ESP32-BLE-Keyboard by T-vK (NimBLE mode)
 * - IRremoteESP8266 by crankyoldgit
 */

#include <Arduino.h>
#include <BleKeyboard.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "pin_definitions.h"

// ===== DEBUG CONFIGURATION =====
#define DEBUG_SERIAL true

// ===== DEBOUNCE SETTINGS =====
#define DEBOUNCE_DELAY_MS   50
#define VOLUME_REPEAT_DELAY 200  // Faster repeat for volume buttons

// ===== IR =====
// The TV power code the Power button replays.
#define TV_POWER_IR_CODE  0xB9467D02
#define TV_POWER_IR_BITS  32

// ===== GLOBAL OBJECTS =====
BleKeyboard bleKeyboard("OpenMote", "OpenMote.io", 100);
IRsend irsend(IR40);   // IR transmitter on GPIO 41

// ===== STATE VARIABLES =====
bool wasConnected = false;      // BLE

// Button state tracking
bool powerButtonPressed = false;
bool homeButtonPressed = false;
bool aButtonPressed = false;
bool bButtonPressed = false;
bool plusButtonPressed = false;
bool minusButtonPressed = false;
bool dpadUpPressed = false;
bool dpadDownPressed = false;
bool dpadLeftPressed = false;
bool dpadRightPressed = false;

unsigned long lastDebounceTime = 0;
unsigned long lastVolumeRepeatTime = 0;

// ===== HARDWARE HELPER FUNCTIONS =====

// LED Control
void turnOnLED1() { digitalWrite(LED_1, HIGH); }
void turnOffLED1() { digitalWrite(LED_1, LOW); }
void turnOnLED2() { digitalWrite(LED_2, HIGH); }
void turnOffLED2() { digitalWrite(LED_2, LOW); }
void turnOnLED3() { digitalWrite(LED_3, HIGH); }
void turnOffLED3() { digitalWrite(LED_3, LOW); }
void turnOnLED4() { digitalWrite(LED_4, HIGH); }
void turnOffLED4() { digitalWrite(LED_4, LOW); }

void turnOnAllLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], HIGH);
  }
}

void turnOffAllLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}

// Button Reading (active LOW with internal pullup)
bool isPowerButtonPressed() { return digitalRead(POWER_BUTT) == LOW; }
bool isHomeButtonPressed() { return digitalRead(HOME_BUTT) == LOW; }
bool isAButtonPressed() { return digitalRead(A_BUTT) == LOW; }
bool isBButtonPressed() { return digitalRead(B_BUTT) == LOW; }
bool isPlusButtonPressed() { return digitalRead(PLUS_BUTT) == LOW; }
bool isMinusButtonPressed() { return digitalRead(MINUS_BUTT) == LOW; }
bool isDPadUpPressed() { return digitalRead(UP_BUTT) == LOW; }
bool isDPadDownPressed() { return digitalRead(DOWN_BUTT) == LOW; }
bool isDPadLeftPressed() { return digitalRead(LEFT_BUTT) == LOW; }
bool isDPadRightPressed() { return digitalRead(RIGHT_BUTT) == LOW; }

// ===== HARDWARE INITIALIZATION =====
void initHardware() {
  // Initialize LED pins as outputs
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  // Rumble is unused, but still drive the pin LOW: RMBL is the gate of the
  // SI2302 (Q1), and leaving it floating lets the motor buzz.
  pinMode(RMBL, OUTPUT);
  digitalWrite(RMBL, LOW);

  // Initialize button pins with internal pullup
  pinMode(POWER_BUTT, INPUT_PULLUP);
  pinMode(HOME_BUTT, INPUT_PULLUP);
  pinMode(A_BUTT, INPUT_PULLUP);
  pinMode(B_BUTT, INPUT_PULLUP);
  pinMode(PLUS_BUTT, INPUT_PULLUP);
  pinMode(MINUS_BUTT, INPUT_PULLUP);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(UP_BUTT, INPUT_PULLUP);
  pinMode(DOWN_BUTT, INPUT_PULLUP);
  pinMode(LEFT_BUTT, INPUT_PULLUP);
  pinMode(RIGHT_BUTT, INPUT_PULLUP);
}

// ===== FEEDBACK FUNCTIONS =====

void showButtonFeedback(int buttonType) {
  switch(buttonType) {
    case 1:  // Power - All LEDs pulse
      turnOnAllLEDs();
      delay(100);
      turnOffAllLEDs();
      break;
    case 2:  // Select - LED1 & LED3 diagonal
      turnOnLED1();
      turnOnLED3();
      delay(80);
      turnOffLED1();
      turnOffLED3();
      break;
    case 3:  // Navigation - Single LED flash
      turnOnLED2();
      delay(60);
      turnOffLED2();
      break;
    case 4:  // Volume Up - LED1 & LED2
      turnOnLED1();
      turnOnLED2();
      delay(70);
      turnOffLED1();
      turnOffLED2();
      break;
    case 5:  // Volume Down - LED3 & LED4
      turnOnLED3();
      turnOnLED4();
      delay(70);
      turnOffLED3();
      turnOffLED4();
      break;
    case 6:  // Back - LED2 & LED4 diagonal
      turnOnLED2();
      turnOnLED4();
      delay(80);
      turnOffLED2();
      turnOffLED4();
      break;
    case 7:  // Home - All LEDs quick pulse
      turnOnAllLEDs();
      delay(120);
      turnOffAllLEDs();
      break;
  }
}

// ===== SETUP =====
void setup() {
  // Initialize Serial FIRST with longer delay for USB CDC
  Serial.begin(115200);
  delay(3000);  // Longer delay for USB CDC to fully enumerate

  Serial.println("\n\n\n");
  Serial.println("==========================================");
  Serial.println("OpenMote Gen 3.3 - BLE TV Remote");
  Serial.println("==========================================");
  Serial.flush();

  Serial.println("Initializing hardware...");
  initHardware();
  Serial.println("Hardware initialized");
  Serial.flush();

  Serial.println("Initializing IR transmitter...");
  irsend.begin();
  Serial.println("IR transmitter initialized (38kHz NEC protocol)");
  Serial.flush();

  Serial.println("Starting BLE Keyboard...");
  bleKeyboard.begin();
  Serial.println("BLE Keyboard started!");
  Serial.flush();

  Serial.println("==========================================");
  Serial.println("Button Mappings:");
  Serial.println("  Power:    IR TV power code");
  Serial.println("  A/B/Home/DPad/+/-: BLE TV controls");
  Serial.println("==========================================");
  Serial.println("Setup complete! Waiting for button presses...");
  Serial.println("");
  Serial.flush();

  // Startup sequence - cascade LEDs
  turnOnLED1();
  delay(100);
  turnOnLED2();
  delay(100);
  turnOnLED3();
  delay(100);
  turnOnLED4();
  delay(100);
  turnOffAllLEDs();

}

// ===== MAIN LOOP =====
void loop() {
  unsigned long currentTime = millis();

  // Check connection status changes
  if (bleKeyboard.isConnected()) {
    if (!wasConnected) {
      wasConnected = true;
      #if DEBUG_SERIAL
      Serial.println(">>> Fire TV Connected!");
      #endif

      // Visual feedback: Quick LED flash
      turnOnAllLEDs();
      delay(100);
      turnOffAllLEDs();
    }

    // ===== A BUTTON - SELECT/OK =====
    bool aState = isAButtonPressed();
    if (aState && !aButtonPressed) {
      if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY_MS) {
        #if DEBUG_SERIAL
        Serial.println(">>> Select (Enter)");
        #endif
        bleKeyboard.write(KEY_RETURN);
        showButtonFeedback(2);

        aButtonPressed = true;
        lastDebounceTime = currentTime;
      }
    } else if (!aState) {
      aButtonPressed = false;
    }

    // ===== B BUTTON - BACK =====
    bool bState = isBButtonPressed();
    if (bState && !bButtonPressed) {
      if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY_MS) {
        #if DEBUG_SERIAL
        Serial.println(">>> Back (Escape)");
        #endif
        bleKeyboard.write(KEY_ESC);
        showButtonFeedback(6);

        bButtonPressed = true;
        lastDebounceTime = currentTime;
      }
    } else if (!bState) {
      bButtonPressed = false;
    }

    // ===== HOME BUTTON - HOME MENU =====
    bool homeState = isHomeButtonPressed();
    if (homeState && !homeButtonPressed) {
      if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY_MS) {
        #if DEBUG_SERIAL
        Serial.println(">>> Home");
        #endif
        // Fire TV Home button - try consumer control
        bleKeyboard.press(KEY_MEDIA_WWW_HOME);
        delay(50);
        bleKeyboard.release(KEY_MEDIA_WWW_HOME);
        showButtonFeedback(7);

        homeButtonPressed = true;
        lastDebounceTime = currentTime;
      }
    } else if (!homeState) {
      homeButtonPressed = false;
    }

    // ===== PLUS BUTTON - VOLUME UP =====
    bool plusState = isPlusButtonPressed();
    if (plusState) {
      // Allow repeat for volume (hold button = continuous volume change)
      if (!plusButtonPressed || (currentTime - lastVolumeRepeatTime >= VOLUME_REPEAT_DELAY)) {
        if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY_MS) {
          #if DEBUG_SERIAL
          Serial.println(">>> Volume Up");
          #endif
          bleKeyboard.press(KEY_MEDIA_VOLUME_UP);
          delay(50);
          bleKeyboard.release(KEY_MEDIA_VOLUME_UP);
          showButtonFeedback(4);

          plusButtonPressed = true;
          lastDebounceTime = currentTime;
          lastVolumeRepeatTime = currentTime;
        }
      }
    } else {
      plusButtonPressed = false;
    }

    // ===== MINUS BUTTON - VOLUME DOWN =====
    bool minusState = isMinusButtonPressed();
    if (minusState) {
      // Allow repeat for volume (hold button = continuous volume change)
      if (!minusButtonPressed || (currentTime - lastVolumeRepeatTime >= VOLUME_REPEAT_DELAY)) {
        if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY_MS) {
          #if DEBUG_SERIAL
          Serial.println(">>> Volume Down");
          #endif
          bleKeyboard.press(KEY_MEDIA_VOLUME_DOWN);
          delay(50);
          bleKeyboard.release(KEY_MEDIA_VOLUME_DOWN);
          showButtonFeedback(5);

          minusButtonPressed = true;
          lastDebounceTime = currentTime;
          lastVolumeRepeatTime = currentTime;
        }
      }
    } else {
      minusButtonPressed = false;
    }

    // ===== D-PAD UP - NAVIGATE UP =====
    bool dpadUpState = isDPadUpPressed();
    if (dpadUpState && !dpadUpPressed) {
      if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY_MS) {
        #if DEBUG_SERIAL
        Serial.println(">>> Navigate Up");
        #endif
        bleKeyboard.write(KEY_UP_ARROW);
        showButtonFeedback(3);

        dpadUpPressed = true;
        lastDebounceTime = currentTime;
      }
    } else if (!dpadUpState) {
      dpadUpPressed = false;
    }

    // ===== D-PAD DOWN - NAVIGATE DOWN =====
    bool dpadDownState = isDPadDownPressed();
    if (dpadDownState && !dpadDownPressed) {
      if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY_MS) {
        #if DEBUG_SERIAL
        Serial.println(">>> Navigate Down");
        #endif
        bleKeyboard.write(KEY_DOWN_ARROW);
        showButtonFeedback(3);

        dpadDownPressed = true;
        lastDebounceTime = currentTime;
      }
    } else if (!dpadDownState) {
      dpadDownPressed = false;
    }

    // ===== D-PAD LEFT - NAVIGATE LEFT =====
    bool dpadLeftState = isDPadLeftPressed();
    if (dpadLeftState && !dpadLeftPressed) {
      if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY_MS) {
        #if DEBUG_SERIAL
        Serial.println(">>> Navigate Left");
        #endif
        bleKeyboard.write(KEY_LEFT_ARROW);
        showButtonFeedback(3);

        dpadLeftPressed = true;
        lastDebounceTime = currentTime;
      }
    } else if (!dpadLeftState) {
      dpadLeftPressed = false;
    }

    // ===== D-PAD RIGHT - NAVIGATE RIGHT =====
    bool dpadRightState = isDPadRightPressed();
    if (dpadRightState && !dpadRightPressed) {
      if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY_MS) {
        #if DEBUG_SERIAL
        Serial.println(">>> Navigate Right");
        #endif
        bleKeyboard.write(KEY_RIGHT_ARROW);
        showButtonFeedback(3);

        dpadRightPressed = true;
        lastDebounceTime = currentTime;
      }
    } else if (!dpadRightState) {
      dpadRightPressed = false;
    }

  } else {
    // Not connected - reset connection state and button states
    if (wasConnected) {
      wasConnected = false;
      #if DEBUG_SERIAL
      Serial.println(">>> Fire TV Disconnected!");
      Serial.println("Waiting for reconnection...");
      #endif
    }

    // Only the BLE buttons are gated on the BLE link. The Power button (IR)
    // keeps working while disconnected.
    homeButtonPressed = false;
    aButtonPressed = false;
    bButtonPressed = false;
    plusButtonPressed = false;
    minusButtonPressed = false;
    dpadUpPressed = false;
    dpadDownPressed = false;
    dpadLeftPressed = false;
    dpadRightPressed = false;

    // Show BLE disconnected status with LED2 slow blink
    static unsigned long lastBlinkTime = 0;
    static bool blinkState = false;
    if (currentTime - lastBlinkTime >= 1000) {
      blinkState = !blinkState;
      if (blinkState) {
        turnOnLED2();
      } else {
        turnOffLED2();
      }
      lastBlinkTime = currentTime;
    }
  }

  // ===== POWER BUTTON - REPLAY THE TV POWER IR CODE (works with BLE down) =====
  bool powerState = isPowerButtonPressed();
  if (powerState && !powerButtonPressed) {
    if (currentTime - lastDebounceTime >= DEBOUNCE_DELAY_MS) {
      Serial.printf(">>> POWER BUTTON - sending IR code 0x%08X\n", TV_POWER_IR_CODE);
      Serial.flush();

      irsend.sendNEC(TV_POWER_IR_CODE, TV_POWER_IR_BITS);

      showButtonFeedback(1);

      powerButtonPressed = true;
      lastDebounceTime = currentTime;
    }
  } else if (!powerState) {
    powerButtonPressed = false;
  }

  delay(10);
}
