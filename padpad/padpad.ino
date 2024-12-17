#include <TinyUSB_Mouse_and_Keyboard.h>
#include <hardware/flash.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Keypad.h>

#include "config.h"
#include "serial.h"
#include "button.h"
#include "helpers.h"

Adafruit_Keypad buttons = Adafruit_Keypad(makeKeymap(generateKeymap(ROWS, COLS)), ROW_PINS, COL_PINS, ROWS, COLS);
#if !LED_DISABLED
Adafruit_NeoPixel led(1, LED_PIN);
#endif
#if !JOYSTICK_DISABLED
Button joystick_button(JOYSTICK_PIN_BUTTON);
#endif

Memory memory;

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

  // HID simulation
  Keyboard.begin();
  Mouse.begin();

  buttons.begin();

  // Load configuration from flash
  loadMemory();

  // Explicitly set the analog resolution
  // Make sure to set `CUSTOM_ADC_RESOLUTION` if you changed this
  analogReadResolution(10);

#if !LED_DISABLED
  led.begin();
  led.setBrightness(30);
  ledSetColor(0, 0, 0);
#endif

#if !POTENTIOMETERS_DISABLED
  // Initially, we set all members to -1 for the logic of analogStepRead()
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

#if !JOYSTICK_DISABLED
  // Setup the joystick pins and button
  pinMode(JOYSTICK_PIN_X, INPUT);
  pinMode(JOYSTICK_PIN_Y, INPUT);
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

  handleButtons();
  handleJoystick();

  pairCheck();

  // pairCheck() decides if the rest of the code should be ignored or not
  if (should_skip)
    return;

  handleMessages();

#if !POTENTIOMETERS_DISABLED
  handlePotentiometers();
#endif

#if !ROTARY_ENCODER_DISABLED
  handleRotaryEncoder();
#endif
}


// ------------- Config memory functions ------------ //

void loadMemory() {
  const uint8_t* flash = (const uint8_t*)(XIP_BASE + FLASH_END_OFFSET);

  // Copy flash data into the config memory
  memcpy(&memory, flash, sizeof(Memory));

  // Validate config memory data by comparing the `sector_check`
  if (memory.sector_check != SECTOR_CHECK) {
    memory = default_memory;  // Set to default config memory

    serialSend("MEMORY", "Could not retrieve config memory data! Using the defaults instead...");
  }
}

// Microcontrollers have limited amount of write/erase cycles
// e.g. RPI Pico that we use here has about ~10,000 cycles!
void saveMemory() {
  // Disable interrupts for flash operations
  noInterrupts();

  // Erase flash sector (required before writing)
  flash_range_erase(FLASH_END_OFFSET, FLASH_SECTOR_SIZE);

  // Write memory struct to flash
  flash_range_program(FLASH_END_OFFSET, (uint8_t*)&memory, sizeof(Memory));

  interrupts();  // Re-enable interrupts
}

// Accepted format: 1:97|98;2:99|100;3:101|102;4:103|104;5:105|106;...
// ':' => separator | ';' => button config end
// 1,2,3... = button id in keymap (Started from 1)
// 97|98 => 97 = letter 'a' normal, b = letter 'b' modkey
// letters are in ascii number. e.g. 97 = a
void uploadMemoryToFlash(String memory_string) {
  while (memory_string.length() > 0) {
    int index = memory_string.indexOf(MESSAGE_END);

    if (index == -1) {
      // Couldn't find anymore `MESSAGE_END` character
      index = memory_string.length();
    }

    String current = memory_string.substring(0, index);

    int separator_position = current.indexOf(MESSAGE_SEP, 0);

    if (separator_position == -1) {
      // Format of memory_string isn't correct (At least for current button)
      serialError("Uploading to memory failed, memory_string format is incorrect!");

      return;
    }

    uint8_t number = current.substring(0, separator_position).toInt();
    String keys = current.substring(separator_position + 1);

    int inner_separator_position = keys.indexOf(MESSAGE_SEP_INNER, 0);

    if (inner_separator_position == -1) {
      // Format of memory_string isn't correct (At least for current button)
      serialError("Uploading to memory failed, memory_string format is incorrect!");

      return;
    }

    byte normal_letter = keys.substring(0, inner_separator_position).toInt();
    byte modkey_letter = keys.substring(inner_separator_position + 1).toInt();

    // Button indecies in keymap start from 1 but in buttons_layout,
    // they start from 0. Hence the "number - 1"
    memory.buttons_layout[number - 1].key = normal_letter;
    memory.buttons_layout[number - 1].mod = modkey_letter;

    if (index == memory_string.length()) {
      // Last index
      serialError("Last index");

      break;
    }

    memory_string = memory_string.substring(index + 1);
  }

  // Upload to flash
  saveMemory();
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
      // Uploading to config memory
      case 'u':
        uploadMemoryToFlash(incoming_message.value);

        break;

      // Test message to change LED's color
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

    byte key = memory.buttons_layout[id].key;
    byte key_orig = key;
    byte mod = memory.buttons_layout[id].mod;

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

void handleJoystick() {
#if !JOYSTICK_DISABLED
  static unsigned long last_move_time = 0;

  // Accumulated movement values for smooth fractional handling
  static float accumulated_x = 0.0;
  static float accumulated_y = 0.0;

  unsigned long current_time = millis();

  // Only trigger joystick movement if enough time has passed
  if ((current_time - last_move_time) >= JOYSTICK_TRIGGER_INTERVAL) {
    // Read joystick values
    int raw_x = analogRead(JOYSTICK_PIN_X);
    int raw_y = analogRead(JOYSTICK_PIN_Y);

    // Joystick idle position values (Almost 0)
    int value_x = raw_x - (CUSTOM_ADC_RESOLUTION / 2);
    int value_y = raw_y - (CUSTOM_ADC_RESOLUTION / 2);

    // Apply deadzone filtering to ignore ADC noise and cross-talk
    if (abs(value_x) < JOYSTICK_DEADZONE) value_x = 0;
    if (abs(value_y) < JOYSTICK_DEADZONE) value_y = 0;

    // Multiplication of 25 is just so the sensitivity range can be lower
    // so, intead of 0-25 it's 0-1 now. 0.1, 0.2, ...
    accumulated_x += value_x * joystick_sensitivity / 25;
    accumulated_y += value_y * joystick_sensitivity / 25;

    int mouse_x = int(accumulated_x);
    int mouse_y = int(accumulated_y);

    // Subtract the already moved amount from the acculumator for next cycle
    accumulated_x -= mouse_x;
    accumulated_y -= mouse_y;

    if (mouse_x != 0 || mouse_y != 0) {
      Mouse.move(mouse_x, mouse_y);
    }

    last_move_time = current_time;
  }

  joystick_button.tick();

  if (joystick_button.pressed()) {
    Mouse.press();
  } else if (joystick_button.released()) {
    Mouse.release();
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
