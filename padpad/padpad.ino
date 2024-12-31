#include <TinyUSB_Mouse_and_Keyboard.h>
#include <hardware/flash.h>
#include <Adafruit_Keypad.h>
#include <Adafruit_NeoPixel.h>
#include <U8g2lib.h>

#include "icons.h"
#include "config.h"
#include "serial.h"
#include "button.h"
#include "helpers.h"
#include "menus.h"

Adafruit_Keypad buttons = Adafruit_Keypad(makeKeymap(generateKeymap(ROWS, COLS)), ROW_PINS, COL_PINS, ROWS, COLS);
#if !LED_DISABLED
Adafruit_NeoPixel led(1, LED_PIN);
#endif
#if !JOYSTICK_DISABLED
Button joystick_button(JOYSTICK_PIN_BUTTON);
#endif
#if !ROTARY_ENCODER_DISABLED
Button rotary_encoder_button(ROTARY_ENCODER_BUTTON);
#endif
#if !DISPLAY_DISABLED
U8G2_ST7920_128X64_F_SW_SPI display(U8G2_R0, DISPLAY_E_PIN, DISPLAY_RW_PIN, DISPLAY_RS_PIN, DISPLAY_RST_PIN);
#endif

Memory memory;

Message incoming_message = {
  .message = "",
  .key = 0,
  .value = "",
};

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

#if !DISPLAY_DISABLED
ViewType current_view = VIEW_HOME;
Menu current_menu = {
  .items = main_menu,
  .size = ARRAY_SIZE(main_menu),
  .index = 0,
  .offset = 0,
  .last_menu = nullptr
};
DynamicRef page_data;

unsigned long display_view_timeout = DISPLAY_DEFAULT_VIEW_TIMEOUT;

int8_t menu_arrow_state = 0;  // -1 = up, 1 = down, 0 = none
#endif

#if MULTI_CORE_OPERATIONS
volatile bool update_display = true;  // Flag to request display updates
volatile bool display_ready = true;   // Ensure one update at a time
volatile bool core1_paused = false;
#endif

void setup() {
  TinyUSBDevice.setProductDescriptor(DEVICE_NAME);
  TinyUSBDevice.setManufacturerDescriptor(DEVICE_MANUFACTURER);

  // HID simulation
  Keyboard.begin();
  Mouse.begin();

  buttons.begin();

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

#if !DISPLAY_DISABLED
  display.begin();

  requestDisplayUpdate();
#endif

  Serial.begin(BAUD_RATE);

  // Load configuration from flash
  loadMemory();

  multicore_launch_core1(core1_entry);
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

  // Functionalities that work without needing device to be paired
  handleButtons();
  handleJoystick();
  handleRotaryEncoder();
  handleDisplay();

  pairCheck();

  // pairCheck() decides if the rest of the code should be ignored or not
  if (should_skip)
    return;

  // Functionalities that need device to be paired
  handleMessages();
  handlePotentiometers();
}

// Second core main funtion (Core 1)
void core1_entry() {
#if MULTI_CORE_OPERATIONS
#if !DISPLAY_DISABLED
  unsigned long last_update = millis();

  while (true) {
    if (core1_paused) {
      // Core 1 sleeps until Core 0 signals
      __wfe();  // Wait for event
    }

    // Automatic screen update every second
    if (millis() - last_update >= DISPLAY_AUTO_UPDATE_INTERVAL) {
      last_update = millis();

      drawViews();
    }

    if (update_display && display_ready) {
      display_ready = false;
      update_display = false;

      drawViews();

      display_ready = true;
    }
  }
#endif
#endif
}

void requestDisplayUpdate() {
#if !DISPLAY_DISABLED
#if MULTI_CORE_OPERATIONS
  update_display = true;
#else
  drawViews();
#endif
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

    serialSend("MEMORY", "Could not retrieve config memory data! Resetting to defaults...");

    saveMemory();

    return;
  }

  serialSend("MEMORY", "Config memory data was loaded.");
}

// Microcontrollers have limited amount of write/erase cycles
// e.g. RPI Pico that we use here has about ~10,000 cycles!
void saveMemory() {
#if MULTI_CORE_OPERATIONS
  core1_paused = true;
  __sev();  // Signal event to wake Core 1 so WDT doesn't reset the MC
#endif

  // Disable interrupts for flash operations
  noInterrupts();

  // Erase flash sector (required before writing)
  flash_range_erase(FLASH_END_OFFSET, FLASH_SECTOR_SIZE);

  // Write memory struct to flash
  flash_range_program(FLASH_END_OFFSET, (uint8_t*)&memory, sizeof(Memory));

  interrupts();  // Re-enable interrupts

#if MULTI_CORE_OPERATIONS
  core1_paused = false;
  __sev();  // Ensure Core 1 sees the resume
#endif
}

// Accepted format: 1:97|98;2:99|100;3:101|102;4:103|104;5:105|106;...
// ':' => separator, ';' => button config end
// 1,2,3... = button id in keymap (Started from 1)
// 97|98 => 97 = letter 'a' normal, b = letter 'b' modkey
// letters are in ascii number. e.g. 97 = a
// TODO: Rename to update buttons
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

    if ((millis() - pairingTimer) >= PAIR_CHECK_INTERVAL) {
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

// TODO: Rename to keyboard.
void handleButtons() {
  if (!memory.keyboard_enabled) return;

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
  if (!memory.potentiometers_enabled) return;

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
  if (!memory.joystick_mouse_enabled) return;

  // Multiplication of 25 is just so the sensitivity range can be lower
  // so, intead of 0-25 it's 0-1 now. 0.1, 0.2, ...
  const float sensitivity = memory.joystick_sensitivity / 25;

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

    accumulated_x += value_x * sensitivity;
    accumulated_y += value_y * sensitivity;

    int mouse_x = int(accumulated_x);
    int mouse_y = int(accumulated_y);

    // Subtract the already moved amount from the acculumator for next cycle
    accumulated_x -= mouse_x;
    accumulated_y -= mouse_y;

    if (mouse_x != 0 || mouse_y != 0) {
      int mouse_wheel_y = 0;

#if !MODKEY_DISABLED
      if (modkey) {
        // Use mouse scroll instead of moving cursor if the modkey was held
        int max_mouse_y = (CUSTOM_ADC_RESOLUTION / 2) * sensitivity;

        if (mouse_y != 0) {
          mouse_wheel_y = mouse_y > 0 ? -2 : 2;

          // If current was less than half of max, lower scroll speed
          // essentially, two steps of scrolling speed
          if (abs(mouse_y) < max_mouse_y) mouse_wheel_y /= 2;
        }

        // Ignore mouse movement
        mouse_x = 0;
        mouse_y = 0;
      }
#endif

      Mouse.move(mouse_x, mouse_y, mouse_wheel_y);
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
  if (rotaryEncoderMoved()) {
    int8_t rotation_value = rotaryEncoderRead();

    // If the movement was valid, do something
    if (rotation_value != 0) {
      static unsigned long last_moved_time = 0;
      unsigned long current_time = millis();

      if ((current_time - last_moved_time) < ROTARY_ENCODER_TRIGGER_DELAY)
        return;

      last_moved_time = current_time;

      // Check rotary encoder direction
      if (rotation_value < 1) {
#if DEBUG_ROTARY_ENCODER
        Serial.println("counterclockwise");
#endif
        rotaryEncoderCounterclockwise();
      } else {
#if DEBUG_ROTARY_ENCODER
        Serial.println("clockwise");
#endif
        rotaryEncoderClockwise();
      }

#if DEBUG_ROTARY_ENCODER
      Serial.print(encoder_test);
      Serial.print("\t");
      Serial.println(rotation_value);
#endif
    }
  }

  rotary_encoder_button.tick();

  if (rotary_encoder_button.pressed()) {
    rotaryEncoderButton();
  }
#endif
}

void handleDisplay() {
#if !DISPLAY_DISABLED
  if (current_view != VIEW_PAGE && page_data.getType() != DynamicRef::None) {
    page_data.reset();
  }

  // Reset `Menu` view's arrow state
  if (current_view == VIEW_MENU && millis() - menuGetLastInteractionTime() > DISPLAY_AUTO_UPDATE_INTERVAL) {
    menu_arrow_state = 0;
  }

  // Return to `Home` view after timeout
  if (current_view != VIEW_HOME && millis() - menuGetLastInteractionTime() > display_view_timeout) {
    goToHome();

#if !MULTI_CORE_OPERATIONS
    requestDisplayUpdate();
#endif
  }
#endif
}

#if !ROTARY_ENCODER_DISABLED
void rotaryEncoderClockwise() {
#if !DISPLAY_DISABLED
  menuResetInteractionTime();

  switch (current_view) {
    case VIEW_MENU:
      menuDown();

      break;
  }

  requestDisplayUpdate();
#endif
}

void rotaryEncoderCounterclockwise() {
#if !DISPLAY_DISABLED
  menuResetInteractionTime();

  switch (current_view) {
    case VIEW_MENU:
      menuUp();

      break;
  }

  requestDisplayUpdate();
#endif
}

void rotaryEncoderButton() {
#if !DISPLAY_DISABLED
  menuResetInteractionTime();

  switch (current_view) {
    case VIEW_HOME:
      goToMainMenu();

      break;

    case VIEW_MENU:
      menuSelect();

      break;
  }

  menu_arrow_state = 0;

  // TEST: Just to demenstrate page_data
  if (current_view == VIEW_PAGE) {
    if (page_data == true) {
      // memory.joystick_mouse_enabled = false;
      page_data = false;
    } else {
      // memory.joystick_mouse_enabled = true;
      page_data = true;
    }
  }

  requestDisplayUpdate();
#endif
}
#endif

void drawViews() {
#if !DISPLAY_DISABLED
  display.clearBuffer();
  display.setDrawColor(1);

  switch (current_view) {
    case VIEW_HOME:
      drawHomeView();

      break;

    case VIEW_MENU:
      drawMenuView();

      break;

    case VIEW_PAGE:
      drawPageView();

      break;
  }

  display.sendBuffer();
#endif
}

void drawHomeView() {
#if !DISPLAY_DISABLED
  // Buffer clearing is handled by `drawViews()`
  display.setDrawColor(1);
  display.setFont(u8g2_font_ncenB14_tr);
  display.drawStr(10, 32, "PadPad");
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(10, 48, "IrregularCelery");
  display.setCursor(96, 24);
  display.print(analogReadTemp());
  // Sending buffer is handled by `drawViews()`
#endif
}

void drawMenuView() {
#if !DISPLAY_DISABLED
  display.setFontMode(1);
  display.setBitmapMode(1);
  display.setFont(u8g2_font_6x10_tr);

  MenuItem* items = current_menu.items;
  int size = current_menu.size;
  int index = current_menu.index;
  int offset = current_menu.offset;

  const int frame_title_padding = 8;

  // Current menu item icon
  display.setDrawColor(1);
  display.drawXBMP(DISPLAY_WIDTH - ICON_WIDTH - DISPLAY_PADDING,
                   (DISPLAY_HEIGHT / 2) - (ICON_HEIGHT / 2),
                   ICON_WIDTH, ICON_HEIGHT, items[index].icon);

  // Draw arrows
  display.drawXBMP(DISPLAY_WIDTH - ICON_WIDTH - DISPLAY_PADDING,
                   DISPLAY_PADDING, ICON_WIDTH, ICON_HEIGHT,
                   menu_arrow_state == -1 ? arrow_up_filled : arrow_up);

  display.drawXBMP(DISPLAY_WIDTH - ICON_WIDTH - DISPLAY_PADDING,
                   DISPLAY_HEIGHT - ICON_HEIGHT - DISPLAY_PADDING, ICON_WIDTH, ICON_HEIGHT, menu_arrow_state == 1 ? arrow_down_filled : arrow_down);

  for (int i = offset; i < offset + MENU_MAX_VISIBLE_ITEMS && i < size; i++) {
    if (i == index) {
      display.setDrawColor(1);
      display.drawRBox(DISPLAY_PADDING, (i - offset) * (MENU_ITEM_FRAME_HEIGHT + 1) + 1,
                       DISPLAY_WIDTH - ICON_WIDTH - (DISPLAY_PADDING * 3),
                       MENU_ITEM_FRAME_HEIGHT, MENU_ITEM_FRAME_RADIUS);
      display.setDrawColor(0);
    } else {
      display.setDrawColor(1);
      display.drawRFrame(DISPLAY_PADDING, (i - offset) * (MENU_ITEM_FRAME_HEIGHT + 1) + 1,
                         DISPLAY_WIDTH - ICON_WIDTH - (DISPLAY_PADDING * 3),
                         MENU_ITEM_FRAME_HEIGHT, MENU_ITEM_FRAME_RADIUS);
      display.setDrawColor(2);
    }
    display.drawStr(DISPLAY_PADDING + frame_title_padding, (i - offset + 1) * (MENU_ITEM_FRAME_HEIGHT + 1) - 2, items[i].title);
  }
#endif
}

void drawPageView() {
#if !DISPLAY_DISABLED
  const char* title = current_menu.items[current_menu.index].title;

  display.setFont(u8g2_font_6x10_tr);

  int titleWidth = display.getUTF8Width(title);
  int titleHeight = display.getMaxCharHeight();

  int x = (DISPLAY_WIDTH - titleWidth) / 2;
  int y = 10;

  int line_start = 10;

  display.drawStr(x, y, title);
  display.drawHLine(line_start, y + 1, DISPLAY_WIDTH - line_start);

  if (page_data.getType() == DynamicRef::Bool) {
    display.setCursor(20, 20);

    if (page_data == true)
      display.print("On");
    else
      display.print("Off");
  }
#endif
}

// Menu navigation functions

void goToHome() {
#if !DISPLAY_DISABLED
  current_view = VIEW_HOME;
  menu_arrow_state = 0;
#endif
}

#if !DISPLAY_DISABLED
void goToMenu(MenuItem* menu, int menu_size) {
  Menu* new_menu = new Menu;

  *new_menu = current_menu;

  if (current_view == VIEW_MENU) {
    current_menu.last_menu = new_menu;
  } else {
    current_menu.last_menu = nullptr;
  }

  current_menu.items = menu;
  current_menu.size = menu_size;
  current_menu.index = 0;
  current_menu.offset = 0;

  current_view = VIEW_MENU;
}
#endif

void goToPage() {
#if !DISPLAY_DISABLED
  current_view = VIEW_PAGE;
  menu_arrow_state = 0;
#endif
}

void goToMainMenu() {
#if !DISPLAY_DISABLED
  goToMenu(main_menu, ARRAY_SIZE(main_menu));
#endif
}

void menuUp() {
#if !DISPLAY_DISABLED
  menu_arrow_state = -1;

  if (current_menu.index > 0) {
    // Scroll to previous item
    current_menu.index--;

    if (current_menu.index < current_menu.offset) {
      current_menu.offset--;
    }

    return;
  }

  // Scroll to last item
  current_menu.index = current_menu.size - 1;
  current_menu.offset = current_menu.size - min(current_menu.size, MENU_MAX_VISIBLE_ITEMS);
#endif
}

void menuDown() {
#if !DISPLAY_DISABLED
  menu_arrow_state = 1;

  if (current_menu.index < current_menu.size - 1) {
    // Scroll to next item
    current_menu.index++;

    if (current_menu.index >= current_menu.offset + MENU_MAX_VISIBLE_ITEMS) {
      current_menu.offset++;
    }

    return;
  }

  // Scroll to first item
  current_menu.index = 0;
  current_menu.offset = 0;
#endif
}

void menuSelect() {
#if !DISPLAY_DISABLED
  bool continue_after_callback = true;

  int size = current_menu.size;
  int index = current_menu.index;
  MenuItem* items = current_menu.items;

  if (items[index].callback) {
    continue_after_callback = items[index].callback();
  }

  if (!continue_after_callback) return;

  if (items[index].sub_menu) {
    goToMenu(items[index].sub_menu, items[index].sub_menu_size);

    return;
  }

  goToPage();
#endif
}

void menuBack() {
#if !DISPLAY_DISABLED
  if (current_menu.last_menu == nullptr) {
    goToHome();

    return;
  }

  Menu* previous_menu = current_menu.last_menu;
  current_menu = *previous_menu;

  delete previous_menu;
#endif
}

// Menu callback functions
// Should be returning a boolean: After selecting a menu item, if you want
// to call the `callback` function and ignore rest of the items properties
// such as submenu or page view, return false, otherwise return true.
// E.g. `menuGoBack()` function, nothing should happen after callback.

#if !DISPLAY_DISABLED
bool menuGoBack() {
  menuBack();

  return false;
}

bool menuSelectMouse() {
#if !JOYSTICK_DISABLED
  page_data.pointTo(memory.joystick_mouse_enabled);
#endif

  return true;
}

bool menuSelectKeyboard() {
  page_data.pointTo(memory.keyboard_enabled);

  return true;
}

bool menuSelectPotentiometers() {
#if !POTENTIOMETERS_DISABLED
  page_data.pointTo(memory.potentiometers_enabled);
#endif

  return true;
}

bool menuSelectSaveMemory() {
  saveMemory();

  menuBack();

  return false;
}
#endif