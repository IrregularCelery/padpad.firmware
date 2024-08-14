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

  Serial.begin(BAUD_RATE);
}

void loop() {
  while (DEBUG_BUTTONS)
    handleButtons();

  pairCheck();

  // pairCheck() decides if the rest of the code should be ignored or not
  if (should_skip)
    return;

  handleMessages();
  handleButtons();
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
