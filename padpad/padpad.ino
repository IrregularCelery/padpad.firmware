#include <TinyUSB_Mouse_and_Keyboard.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Keypad.h>

#include "config.h"
#include "serial.h"
#include "helpers.h"

#if !LED_DISABLED
Adafruit_NeoPixel led(1, LED_PIN);
#endif
Adafruit_Keypad buttons = Adafruit_Keypad(makeKeymap(generateKeymap(ROWS, COLS)), ROW_PINS, COL_PINS, ROWS, COLS);

Message incoming_message = { "", 0, "" };

bool should_skip = false;
bool paired = false;
#if !LED_DISABLED
bool led_overridden = false;
#endif
bool modkey = false;

#if !POTENTIOMETERS_DISABLED
int potentiometer_values[potentiometers_count] = {};
int last_potentiometer_values[potentiometers_count] = {};
#endif

void setup() {
  TinyUSBDevice.setProductDescriptor(DEVICE_NAME);
  TinyUSBDevice.setManufacturerDescriptor(DEVICE_MANUFACTURER);

  Keyboard.begin();
  buttons.begin();

#if !LED_DISABLED
  led.begin();
  led.setBrightness(30);
  ledSetColor(0, 0, 0);
#endif

#if !POTENTIOMETERS_DISABLED
  // Explicitly set the analog resolution
  analogReadResolution(10);

  // Initially, we set all the members to -1 for the logic of analogStepRead()
  for (int i = 0; i < potentiometers_count; i++) {
    last_potentiometer_values[i] = -1;
  }
#endif

#if !ANALOG_MULTIPLEXER_DISABLED
  // Configure selector pins of the analog multiplexer as outputs
  for (int i = 0; i < 3 /* 3 bits = 8 channels */; i++) {
    pinMode(selector_pins[i], OUTPUT);
  }
#endif

#if !ROTARY_ENCODER_DISABLED
  // Setup the input pins (need to be pulled up with resistors)
  pinMode(ROTARY_ENCODER_PIN1, INPUT);
  pinMode(ROTARY_ENCODER_PIN2, INPUT);

  // Attach both rotary encoder pins to an interrupt pin
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_PIN1), rotaryEncoderMove, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_PIN2), rotaryEncoderMove, CHANGE);
#endif

  Serial.begin(BAUD_RATE);
}

void loop() {
  // ------- DEBUGGING ------- //
  while (DEBUG_BUTTONS) {
    handleButtons();
  }

  while (DEBUG_POTENTIOMETERS) {
    handlePotentiometers();
  }

  while (DEBUG_ROTARY_ENCODER) {
    handleRotaryEncoder();

    yield();
  }

  // ----- END DEBUGGING ----- //

  pairCheck();

  // pairCheck() decides if the rest of the code should be ignored or not
  if (should_skip)
    return;

  handleMessages();
  handleButtons();
#if !POTENTIOMETERS_DISABLED
  handlePotentiometers();
#endif

#if !ROTARY_ENCODER_DISABLED
  handleRotaryEncoder();
#endif
}

void pair() {
  serialSend("PAIRED", "Firmware and device are now connected!");

  paired = true;

  int color[3] = { 0, 255, 0 };

  ledFlash(2, 150, color);
}

void unpair() {
  paired = false;

  int color[3] = { 255, 0, 0 };

  ledFlash(2, 150, color);
}

void pairCheck() {
  if (!paired) {
    if (Serial.available()) {
      String message = Serial.readString();

      if (message == "p1") {
        pair();

        should_skip = true;

        return;
      }
    }

    ledSetColor(0, 0, 255);

    static unsigned long pairingTimer = 0;

    if ((millis() - pairingTimer) >= 1000) {
      serialSend("READY", "Firmware is ready to pair with the app!");

      pairingTimer = millis();
    }

    should_skip = true;

    return;
  }

  if (paired) {
    if (!Serial) {
      unpair();

      should_skip = true;

      return;
    }

#if !LED_DISABLED
    if (!led_overridden)
      ledSetColor(0, 255, 0);
#endif

    should_skip = false;
  }
}

void ledSetColor(int r, int g, int b) {
#if !LED_DISABLED
  if (r != 0 || g != 0 || b != 0)
    led_overridden = true;
  else
    led_overridden = false;

  led.setPixelColor(0, led.Color(r, g, b));
  led.show();
#endif
}

// Duration: delay between each on and off so `delay(duration / 2)`
void ledFlash(int count, int duration, int color[3]) {
#if !LED_DISABLED
  for (int i = 0; i < count; i++) {
    ledSetColor(color[0], color[1], color[2]);
    delay(duration / 2);
    ledSetColor(0, 0, 0);
    delay(duration / 2);
  }
#endif
}

void handleMessages() {
  if (Serial.available()) {
    String message = Serial.readString();

    incoming_message = { message, message.charAt(0), message.substring(1) };

    switch (incoming_message.key) {
      case 'l':
        if (incoming_message.value == "1")
          ledSetColor(255, 255, 0);
        else
          ledSetColor(0, 0, 0);

        break;
    }
  }
}

void handleButtons() {
  static unsigned long buttonTimer = 0;

  if ((millis() - buttonTimer) < DEBOUNCE_TIME)
    return;

  buttonTimer = millis();

  buttons.tick();

  if (buttons.available()) {
    keypadEvent e = buttons.read();

    byte id = e.bit.KEY - 1;
    byte map_id = e.bit.KEY;

    byte key = layout[id].key;
    byte key_orig = key;
    byte mod = layout[id].mod;

#if !MODKEY_DISABLED
    if (modkey && key != 255)
      key = mod;
#endif

    switch (e.bit.EVENT) {
      case KEY_JUST_PRESSED:
#if !MODKEY_DISABLED
        // character 255 = modkey
        if (key == 255)
          modkey = true;
        else
#endif
          Keyboard.press(key);

        if (paired)
          serialSendButton(map_id, modkey, 1);

        break;

      case KEY_JUST_RELEASED:
#if !MODKEY_DISABLED
        // character 255 = modkey
        if (key == 255)
          modkey = false;
        else
#endif
        {
          Keyboard.release(key);

          // There was a bug that if release the button before the modkey,
          // because the modkey is still enabled, the `key` would change
          // to `mod` and thus wouldn't be released here.
          Keyboard.release(key_orig);
        }

        if (mod > 0)
          Keyboard.release(mod);

        if (paired)
          serialSendButton(map_id, modkey, 0);

        break;
    }
  }
}

void handlePotentiometers() {
#if !POTENTIOMETERS_DISABLED
  for (int i = 0; i < potentiometers_count; i++) {

#if !ANALOG_MULTIPLEXER_DISABLED
    // Select the channel
    selectChannel(i);

    int analog_pin = multiplexer_analog_common;
#else
    int analog_pin = potentiometer_pins[i];
#endif

    // Read the analog value from the selected channel
    int value = analogStepRead(analog_pin, &last_potentiometer_values[i]);

    if (value != potentiometer_values[i]) {
      potentiometer_values[i] = value;

      serialSendPotentiometer(i, value);
    }
  }
#endif
}

void handleRotaryEncoder() {
#if !ROTARY_ENCODER_DISABLED
  static int encoder_test = 0;

  if (rotaryEncoderMoved()) {
    int8_t rotation_value = rotaryEncoderRead();

    // If valid movement, do something
    if (rotation_value != 0) {
      // Set any variable value as the rotary encoder position change
      encoder_test += rotation_value;

      // Check rotary encoder direction
      if (rotation_value < 1) {
        Serial.println("counterclockwise");
      } else {
        Serial.println("clockwise");
      }

      Serial.print(encoder_test);
      Serial.print("\t");
      Serial.println(rotation_value);
    }
  }
#endif
}
