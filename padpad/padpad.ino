#include <Adafruit_NeoPixel.h>
#include <Keypad.h>
#include <Keyboard.h>

#include "config.h"

struct Message {
  String message;
  byte key;
  String value;
};

char* generateKeymap(byte row_count, byte col_count) {
  int size = row_count * col_count;
  char* keymap = new char[size];
  char start = 1;  // 1(number) and not '1'(char)

  for (int i = 0; i < size; i++)
    keymap[i] = start + i;

  return keymap;
}

Adafruit_NeoPixel led(1, LED_PIN);
Keypad buttons = Keypad(makeKeymap(generateKeymap(ROWS, COLS)), ROW_PINS, COL_PINS, ROWS, COLS);

bool paired = false;
bool led_overridden = false;

Message incoming_message = { "", 0, "" };

void setup() {
  Serial.begin(BAUD_RATE);

  Keyboard.begin();
  buttons.setDebounceTime(10);

  led.begin();
  led.setBrightness(30);
  ledSetColor(0, 0, 0);
}

void loop() {
  while (DEBUG_BUTTONS)
    handleButtons();

  while (!paired) {
    if (Serial.available()) {
      String message = Serial.readString();

      if (message == "p1") {
        pair();

        break;
      }
    }

    ledSetColor(0, 0, 255);

    serialSend("READY", "Firmware is ready to pair with the app!");

    delay(1000);

    ledSetColor(0, 0, 0);
  }

  if (paired) {
    if (!Serial) {
      unpair();

      return;
    }

    if (!led_overridden)
      ledSetColor(0, 255, 0);
  }

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

void serialSend(String key, String value) {
  Serial.print(key + MESSAGE_SEP + value + MESSAGE_END);
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

void ledSetColor(int r, int g, int b) {
  if (r != 0 || g != 0 || b != 0)
    led_overridden = true;
  else
    led_overridden = false;

  led.setPixelColor(0, led.Color(r, g, b));
  led.show();
}

// Duration: delay between each on and off so `delay(duration / 2)`
void ledFlash(int count, int duration, int color[3]) {
  for (int i = 0; i < count; i++) {
    ledSetColor(color[0], color[1], color[2]);
    delay(duration / 2);
    ledSetColor(0, 0, 0);
    delay(duration / 2);
  }
}

void handleButtons() {
  if (buttons.getKeys())
    for (int i = 0; i < LIST_MAX; i++)
      if (buttons.key[i].stateChanged) {
        byte id = buttons.key[i].kchar;

        switch (buttons.key[i].kstate) {
          case PRESSED:
            Keyboard.press(96 + id);
            serialSend((String)id, "PRESSED");

            break;

          case RELEASED:
            Keyboard.release(96 + id);
            serialSend((String)id, "RELEASED");

            break;
        }
      }
}