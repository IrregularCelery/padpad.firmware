#include <TinyUSB_Mouse_and_Keyboard.h>
#include <hardware/flash.h>
#include <Adafruit_Keypad.h>
#include <Adafruit_NeoPixel.h>
#include <U8g2lib.h>

#include "lib.h"

#include "icons.h"
#include "config.h"
#include "serial.h"
#include "helpers.h"
#include "menus.h"

Adafruit_Keypad buttons = Adafruit_Keypad(makeKeymap(generateKeymap(ROWS, COLS)), ROW_PINS, COL_PINS, ROWS, COLS);
#if !LED_DISABLED
Adafruit_NeoPixel led(1, LED_PIN);
RGBController rgb;
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

#if MULTIPLE_PROFILES
// TODO: Remove test profiles
String profiles[] = { "Internal", "Profile 1", "Profile 2", "Profile 3" };
uint8_t current_profile = 0;
#endif

bool should_skip = false;
bool paired = false;
int current_second = -1;  // Total amount of seconds
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
PageData page_data;

unsigned long display_view_timeout = DISPLAY_DEFAULT_VIEW_TIMEOUT;

int8_t menu_arrow_state = 0;  // -1 = up, 1 = down, 0 = none
#endif

#if MULTI_CORE_OPERATIONS
volatile bool core1_paused = false;
volatile bool core1_busy = false;

volatile bool update_display = true;  // Flag to request display updates
volatile bool display_ready = true;   // Ensure one update at a time
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
  led.setBrightness(50);

  rgb.flash(2, Color(255, 255, 255), 250);
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

#if DEBUG_RESET_MEMORY
  flash_range_erase(FLASH_END_OFFSET, FLASH_SECTOR_SIZE);
#endif

  // Load configuration from flash
  loadMemory();

  // Load internal profiles on startup
  updateProfilesMenu();

#if MULTI_CORE_OPERATIONS
  multicore_launch_core1(core1_entry);
#endif
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
  handleLED();
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
  handleTime();
  handlePotentiometers();
}

// Second core main funtion (Core 1)
#if MULTI_CORE_OPERATIONS
void core1_entry() {
#if !DISPLAY_DISABLED
  auto awaitDrawViews = [&]() {
    core1_busy = true;  // Currently busy
    drawViews();
    core1_busy = false;  // Operation finished
  };

  unsigned long last_update = millis();

  while (true) {
    if (core1_paused) {
      // Core 1 sleeps until Core 0 signals

      core1_busy = false;  // Not busy while paused

      __wfe();  // Wait for event
    }

    // Automatic screen update every second
    if (millis() - last_update >= DISPLAY_AUTO_UPDATE_INTERVAL) {
      last_update = millis();

      awaitDrawViews();
    }

    if (update_display && display_ready) {
      display_ready = false;
      update_display = false;

      awaitDrawViews();

      display_ready = true;
    }
  }
#endif
}
#endif

void requestDisplayUpdate() {
#if !DISPLAY_DISABLED
#if MULTI_CORE_OPERATIONS
  update_display = true;
#else
  drawViews();
#endif
#endif
}

// Recommended: If you're not showing the message from a menu item, Use `goToMessage()` instead.
// If you use `setMessage()` you need to handle it right away using `goToPage()`
// if the message is being set from a menu callback function, you don't need to
// call `goToPage()` Just return `true` in the callback function.
void setMessage(String message, String title = "Message", unsigned long timeout = 0) {
  if (timeout > 0) display_view_timeout = timeout;

  page_data.title = "msg:" + title;
  page_data.description = message;
}

// ------------- Config memory functions ------------ //

void loadMemory() {
  const uint8_t* flash = (const uint8_t*)(XIP_BASE + FLASH_END_OFFSET);

  // Copy flash data into the config memory
  memcpy(&memory, flash, sizeof(Memory));

  // Validate config memory data by comparing the `sector_check`
  if (memory.sector_check != SECTOR_CHECK) {
    // Set to default config memory
    defaultMemory();

#if !DISPLAY_DISABLED
    setMessage("Memory loading failed!\nLoaded defaults instead", "Error", 30 * 1000);
    goToPage();
#endif

    serialSend("MEMORY", "Could not retrieve config memory data! Resetting to defaults...");

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

  // Wait for Core 1 to finish its operation
  while (core1_busy) {
    sleep_us(10);  // To avoid excessive CPU usage
  }
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

#if !DISPLAY_DISABLED
  setMessage("Memory has been saved.", "Completed");
#endif
}

void defaultMemory() {
  memory = default_memory;

  saveMemory();
}

// Accepted format: 1:97|98;2:99|100;3:101|102;4:103|104;5:105|106;...
// ':' => separator, ';' => button config end
// 1,2,3... = button id in keymap (Started from 1)
// 97|98 => 97 = letter 'a' normal, b = letter 'b' modkey
// letters are in ascii number. e.g. 97 = a
void updateMemoryLayout(String memory_string) {
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
}

void pair() {
  serialSend("PAIRED", "Firmware and device are now connected!");

  paired = true;

  requestStartupData();

#if !LED_DISABLED
  rgb.flash(3, Color(0, 255, 0), 75);
#endif
}

void unpair() {
  paired = false;
  current_second = -1;
  current_profile = 0;

#if !LED_DISABLED
  rgb.flash(3, Color(255, 0, 0), 75);
#endif
}

void pairCheck() {
  if (!paired) {
    static String buffer = "";

    if (Serial.available()) {
      char c = Serial.read();

      if (c == '\n') {  // Messages should end with a newline ('\n')
        if (buffer == "p1") {
          pair();

          should_skip = true;

          buffer = "";

          return;
        }

        buffer = "";  // Clear the buffer for the next message
      } else {
        buffer += c;  // Append the character to the buffer
      }
    }

    static unsigned long pairing_timer = 0;

    if ((millis() - pairing_timer) >= PAIR_CHECK_INTERVAL) {
      serialSend("READY", "Firmware is ready to pair with the app!");

      pairing_timer = millis();
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

    should_skip = false;
  }
}

void requestStartupData() {
  serialSend("REQUEST", "STARTUP");
}

void handleMessages() {
  static String buffer = "";

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {  // Incoming messages should end with a newline ('\n')
      Message incoming_message = {
        .message = buffer,
        .key = buffer.charAt(0),
        .value = buffer.substring(1),
      };

      switch (incoming_message.key) {
        // Set current_second
        case 't':
          current_second = incoming_message.value.toInt();

          break;

        // Uploading to config memory
        case 'u':
          updateMemoryLayout(incoming_message.value);

          // TODO: Handle this by a different incoming_message
          // Upload to flash
          saveMemory();

          break;
      }

      buffer = "";  // Clear the buffer for the next message
    } else {
      buffer += c;  // Append the character to the buffer
    }
  }
}

void handleTime() {
  static unsigned long last_millis = 0;
  unsigned long current_millis = millis();

  if (current_millis - last_millis >= 1000) {
    last_millis += 1000;

    current_second++;

    if (current_second >= SECONDS_PER_DAY) {
      current_second = 0;  // Reset at midnight
    }
  }
}

void handleLED() {
#if !LED_DISABLED
  Color current_color = rgb.tick();

  led.setPixelColor(0, led.Color(current_color.r, current_color.g, current_color.b));
  led.show();
#endif
}

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
  if (current_view != VIEW_PAGE && page_data.title.length() > 0) {
    page_data.value.reset();
    page_data.title = "";
    page_data.description = "";
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
    case VIEW_HOME:
      goToProfiles();

      break;

    case VIEW_MENU:
      menuDown();

      break;

    case VIEW_PAGE:
      switch (page_data.value.getType()) {
        case DynamicRef::Bool:
          // Toggle booleans
          page_data.value = !page_data.value;

          break;

        case DynamicRef::Float:
          // Add to value by 0.1
          page_data.value += 0.1f;

          break;

        default:
          break;
      }

      break;
  }

  requestDisplayUpdate();
#endif
}

void rotaryEncoderCounterclockwise() {
#if !DISPLAY_DISABLED
  menuResetInteractionTime();

  switch (current_view) {
    case VIEW_HOME:
      goToProfiles();

      break;

    case VIEW_MENU:
      menuUp();

      break;

    case VIEW_PAGE:
      switch (page_data.value.getType()) {
        case DynamicRef::Bool:
          // Toggle booleans
          page_data.value = !page_data.value;

          break;

        case DynamicRef::Float:
          // Subtract from value by 0.1
          page_data.value -= 0.1f;

          break;

        default:
          break;
      }

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

    case VIEW_PAGE:
      menuBack();

      break;
  }

  menu_arrow_state = 0;

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

void drawStatusPanel() {
#if !DISPLAY_DISABLED
  display.setBitmapMode(1);
  display.setDrawColor(1);

  display.setCursor(0, 10);
  display.print(current_second);

  int max_icon_count = 5;

  int16_t bar_width = (MINI_ICON_WIDTH * max_icon_count)
                      + ((max_icon_count - 1) * 1) /* padding between icons */
                      + (DISPLAY_PADDING * 2);
  int16_t bar_height = MINI_ICON_HEIGHT + DISPLAY_PADDING;
  int16_t bar_x = DISPLAY_WIDTH - bar_width;
  int16_t bar_y = 0;

  int16_t icon_x = MINI_ICON_WIDTH + 1;  // Should be multiplied by position.
                                         // e.g. (2 * icon_x), first one is 0
  int16_t icon_y = DISPLAY_PADDING / 2;

  // Bar line
  display.drawHLine(bar_x, bar_height, bar_width);

  // Vertical separator line
  display.drawVLine(bar_x - 1, 0, DISPLAY_HEIGHT);

  // Icons

  display.drawXBMP(bar_x + DISPLAY_PADDING + (0 * icon_x),
                   icon_y, MINI_ICON_WIDTH, MINI_ICON_HEIGHT,
                   connected_mini_icon);

  // Disabled overlay icon
  if (!paired) {
    display.drawXBMP(bar_x + DISPLAY_PADDING + (0 * icon_x),
                     icon_y, MINI_ICON_WIDTH, MINI_ICON_HEIGHT,
                     not_mini_icon);
    display.setDrawColor(0);
    display.drawXBMP(bar_x + DISPLAY_PADDING + (0 * icon_x),
                     icon_y, MINI_ICON_WIDTH, MINI_ICON_HEIGHT,
                     not_shadow_mini_icon);
    display.setDrawColor(1);
  }

  display.drawXBMP(bar_x + DISPLAY_PADDING + (1 * icon_x),
                   icon_y, MINI_ICON_WIDTH, MINI_ICON_HEIGHT,
                   keyboard_mini_icon);

  // Disabled overlay icon
  if (!memory.keyboard_enabled) {
    display.drawXBMP(bar_x + DISPLAY_PADDING + (1 * icon_x),
                     icon_y, MINI_ICON_WIDTH, MINI_ICON_HEIGHT,
                     not_mini_icon);
    display.setDrawColor(0);
    display.drawXBMP(bar_x + DISPLAY_PADDING + (1 * icon_x),
                     icon_y, MINI_ICON_WIDTH, MINI_ICON_HEIGHT,
                     not_shadow_mini_icon);
    display.setDrawColor(1);
  }

#if !JOYSTICK_DISABLED
  display.drawXBMP(bar_x + DISPLAY_PADDING + (2 * icon_x),
                   icon_y, MINI_ICON_WIDTH, MINI_ICON_HEIGHT,
                   mouse_mini_icon);

  // Disabled overlay icon
  if (!memory.joystick_mouse_enabled) {
    display.drawXBMP(bar_x + DISPLAY_PADDING + (2 * icon_x),
                     icon_y, MINI_ICON_WIDTH, MINI_ICON_HEIGHT,
                     not_mini_icon);
    display.setDrawColor(0);
    display.drawXBMP(bar_x + DISPLAY_PADDING + (2 * icon_x),
                     icon_y, MINI_ICON_WIDTH, MINI_ICON_HEIGHT,
                     not_shadow_mini_icon);
    display.setDrawColor(1);
  }
#endif

  // Draw clock

  char time_str[9];  // 8 chars for HH:MM:SS plus null terminator

  if (paired) {
    int hours = current_second / 3600;
    int minutes = (current_second % 3600) / 60;
    int seconds = current_second % 60;

    sprintf(time_str, "%02d:%02d:%02d", hours, minutes, seconds);
  } else {
    sprintf(time_str, "CLOCK");
  }

  display.setFont(u8g2_font_t0_12b_tr);
  display.drawStr(bar_x + (bar_width / 2) - (display.getStrWidth(time_str) / 2),
                  bar_height + display.getMaxCharHeight() + 1 /* padding */,
                  time_str);
  display.setFont(DISPLAY_DEFAULT_FONT);
#endif
}

void drawHomeView() {
#if !DISPLAY_DISABLED
  // Buffer clearing is handled by `drawViews()`
  drawStatusPanel();
  // Sending buffer is handled by `drawViews()`
#endif
}

void drawMenuView() {
#if !DISPLAY_DISABLED
  display.setFontMode(1);
  display.setBitmapMode(1);
  display.setFont(DISPLAY_DEFAULT_FONT);

  MenuItem* items = current_menu.items;
  int size = current_menu.size;
  int index = current_menu.index;
  int offset = current_menu.offset;

  const int frame_title_padding = 8;

#if DEBUG_DISPLAY
  display.setCursor(DISPLAY_WIDTH - 22, 8);
  display.print(index);
  display.setCursor(DISPLAY_WIDTH - 8, 8);
  display.print(offset);
#endif

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
  bool message_box = false;
  const char* message_indicator = "msg:";

  String title = page_data.title;
  String description = page_data.description;

  // If title contains `message_indicator` AKA "msg:", the page is considered message_box
  int message_prefix_index = title.indexOf(message_indicator);

  if (message_prefix_index != -1) {
    message_box = true;

    title = title.substring(message_prefix_index + strlen(message_indicator));
  }

  int16_t title_width = display.getUTF8Width(title.c_str());

  int16_t title_x = (DISPLAY_WIDTH - title_width) / 2;
  int16_t title_y = 8;

  if (message_box) title_y += DISPLAY_PADDING;

  int16_t line_start = DISPLAY_PADDING * 2;

  display.drawStr(title_x, title_y, title.c_str());

  if (message_box) title_y += 1;

  display.setDrawColor(2);
  display.drawHLine(line_start, title_y + 1, DISPLAY_WIDTH - (line_start * 2));
  display.setDrawColor(1);

  if (message_box) {
    display.setDrawColor(1);
    display.drawRFrame((DISPLAY_PADDING / 2), (DISPLAY_PADDING / 2),
                       DISPLAY_WIDTH - DISPLAY_PADDING,
                       DISPLAY_HEIGHT - DISPLAY_PADDING, 9);
  }

  display.setFont(u8g2_font_spleen5x8_mr);

  int16_t description_padding = 0;

  if (message_box) description_padding = DISPLAY_PADDING;

  int16_t cursor_y = drawWrappedString(description, description_padding + DISPLAY_PADDING,
                                       title_y + DISPLAY_PADDING + 8,
                                       DISPLAY_WIDTH - (DISPLAY_PADDING * 2), 10, 2);

  display.setFont(DISPLAY_DEFAULT_FONT);

  // If the page is a `message_box`, ignore checking `page_data.value`
  if (message_box) {
    // Confirmation button
    int16_t button_width = display.getStrWidth(DISPLAY_CONFIRM_BUTTON_TEXT);
    int16_t button_height = 9;
    int16_t button_x = DISPLAY_WIDTH - button_width - (DISPLAY_PADDING * 2.5);
    int16_t button_y = DISPLAY_HEIGHT - (DISPLAY_PADDING * 2);
    int8_t button_padding = 1;

    display.drawRBox(button_x - (button_padding * 4),
                     button_y - button_height,
                     button_width + (button_padding * 8),
                     button_height + (button_padding * 2), 4);

    display.setDrawColor(0);
    display.drawStr(button_x, button_y, DISPLAY_CONFIRM_BUTTON_TEXT);
    display.setDrawColor(1);

    return;
  }

  switch (page_data.value.getType()) {
    case DynamicRef::Bool:
      {
        int16_t true_button_width = display.getStrWidth(DISPLAY_TRUE_BUTTON_TEXT);
        int16_t false_button_width = display.getStrWidth(DISPLAY_FALSE_BUTTON_TEXT);
        int16_t true_button_x = DISPLAY_WIDTH / 6;
        int16_t false_button_x = DISPLAY_WIDTH - (DISPLAY_WIDTH / 6) - false_button_width;

        int16_t button_height = 9;

        // Selected button
        int16_t button_x = 0;
        int16_t button_y = cursor_y + button_height;
        int16_t button_width = 0;
        int8_t button_padding = 1;

        display.drawStr(true_button_x, button_y, DISPLAY_TRUE_BUTTON_TEXT);
        display.drawStr(false_button_x, button_y, DISPLAY_FALSE_BUTTON_TEXT);

        if (page_data.value == true) {
          button_x = true_button_x;
          button_width = true_button_width;
        } else {
          button_x = false_button_x;
          button_width = false_button_width;
        }

        display.setDrawColor(2);
        display.drawRBox(button_x - (button_padding * 4),
                         button_y - button_height,
                         button_width + (button_padding * 8),
                         button_height + (button_padding * 2), 2);
        display.setDrawColor(1);

        break;
      }

    case DynamicRef::Float:
      {
        const char* float_title = "value: ";

        int16_t value_x = display.getStrWidth(float_title) + DISPLAY_PADDING;
        int16_t value_y = cursor_y + 8;

        int8_t box_height = 9;
        int8_t box_padding = 1;

        display.setFont(u8g2_font_t0_12_tr);
        display.drawStr((DISPLAY_PADDING * 2), value_y, float_title);

        display.setFont(u8g2_font_t0_12b_tr);
        display.setCursor(value_x, value_y);
        display.print((float)(page_data.value));
        display.setDrawColor(1);
        display.setFont(DISPLAY_DEFAULT_FONT);

        break;
      }

    default:
      break;
  }
#endif
}

// Draws a wrapped string and returns last line cursor y
int16_t drawWrappedString(String text, int16_t x, int16_t y, uint16_t max_width, uint8_t line_height, uint8_t space_reduction) {
#if !DISPLAY_DISABLED
  int16_t cursor_x = x;
  int16_t cursor_y = y;

  while (text.length() > 0) {
    int new_line_index = text.indexOf('\n');

    if (new_line_index == 0) {
      cursor_x = x;
      cursor_y += line_height;
      text = text.substring(1);

      continue;
    }

    int space_index = text.indexOf(' ');

    if (new_line_index != -1 && (new_line_index < space_index || space_index == -1)) {
      space_index = new_line_index;
    }

    String word = (space_index == -1) ? text : text.substring(0, space_index);

    int word_width = display.getStrWidth(word.c_str());

    if (cursor_x + word_width > max_width) {
      cursor_x = x;
      cursor_y += line_height;
    }

    // Draw the word
    display.drawStr(cursor_x, cursor_y, word.c_str());

    // Advance the cursor, reducing the space
    cursor_x += word_width + (display.getStrWidth(" ") - space_reduction);

    // Remove the processed word/line and handle \n
    text = (space_index == -1) ? "" : text.substring(space_index + 1);

    if (space_index == new_line_index) {
      cursor_x = x;
      cursor_y += line_height;  // Move to the next line after a newline
    }
  }

  return cursor_y;
#endif
  return 0;
}

void updateProfilesMenu() {
#if !DISPLAY_DISABLED
  if (profiles_menu != nullptr) {
    delete[] profiles_menu;
  }

  int profiles_count = ARRAY_SIZE(profiles);
  profiles_menu = new MenuItem[profiles_count];

  auto callback = []() -> bool {
    int index = 0;

    if (paired) index = current_menu.index;

    current_profile = index;

    return false;
  };

  for (int i = 0; i < profiles_count; i++) {
    profiles_menu[i].title = profiles[i].c_str();
    profiles_menu[i].callback = callback;
  }
#endif
}

// Menu navigation functions

void goToHome() {
#if !DISPLAY_DISABLED
  current_view = VIEW_HOME;
  menu_arrow_state = 0;

  if (display_view_timeout != DISPLAY_DEFAULT_VIEW_TIMEOUT) {
    display_view_timeout = DISPLAY_DEFAULT_VIEW_TIMEOUT;
  }
#endif
}

#if !DISPLAY_DISABLED
void goToMenu(MenuItem* menu, int menu_size, int index = 0) {
  storeLastMenu();

  int offset = index + 1 - MENU_MAX_VISIBLE_ITEMS;

  if (offset < 0) offset = 0;

  current_menu.items = menu;
  current_menu.size = menu_size;
  current_menu.index = index;
  current_menu.offset = offset;


  current_view = VIEW_MENU;
}
#endif

void goToPage() {
#if !DISPLAY_DISABLED
  // To store current_menu as last_menu for going back
  storeLastMenu();

  if (page_data.title.length() == 0) {
    page_data.title = current_menu.items[current_menu.index].title;
  }

  current_view = VIEW_PAGE;
  menu_arrow_state = 0;
#endif
}

void goToMessage(String message, String title = "Message", unsigned long timeout = 0) {
#if !DISPLAY_DISABLED
  setMessage(message, title, timeout);
  goToPage();
#endif
}

void goToMainMenu() {
#if !DISPLAY_DISABLED
  goToMenu(main_menu, ARRAY_SIZE(main_menu));
#endif
}

void goToProfiles() {
#if !DISPLAY_DISABLED
  if (current_view != VIEW_HOME) return;

  if (!paired) {
    goToMessage("Cannot load profiles!\nDevice isn't paired yet", "Error", 1000);

    return;
  }

  goToMenu(profiles_menu, ARRAY_SIZE(profiles), current_profile);
#endif
}

void storeLastMenu() {
#if !DISPLAY_DISABLED
  Menu* new_menu = new Menu;

  *new_menu = current_menu;

  if (current_view == VIEW_MENU) {
    current_menu.last_menu = new_menu;
  } else {
    current_menu.last_menu = nullptr;
  }
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

  // If callback returned false, go back in menu
  if (!continue_after_callback) return menuBack();

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

  if (current_view != VIEW_MENU) {
    current_view = VIEW_MENU;
  }

  Menu* previous_menu = current_menu.last_menu;
  current_menu = *previous_menu;

  delete previous_menu;
#endif
}

// Menu callback functions
// Should be returning a boolean: After selecting a menu item, if you want
// to call the `callback` function and go back to previous menu,
// return `false`. otherwise return `true`.
// NOTE: Most return `true` if the callback has `setMessage()`

#if !DISPLAY_DISABLED
bool menuSelectBack() {
  return false;
}

bool menuSelectMouseSensitivity() {
#if !JOYSTICK_DISABLED
  page_data.value.pointTo(memory.joystick_sensitivity);
  page_data.description = "Adjust mouse sensitivity";
#endif

  return true;
}

bool menuSelectMouseToggle() {
#if !JOYSTICK_DISABLED
  page_data.value.pointTo(memory.joystick_mouse_enabled);
  page_data.description = "Joystick mouse emulation\n(Hold ModKey to scroll)";
#endif

  return true;
}

bool menuSelectKeyboard() {
  page_data.value.pointTo(memory.keyboard_enabled);
  page_data.description = "Capture keyboard inputs";

  return true;
}

bool menuSelectPotentiometers() {
#if !POTENTIOMETERS_DISABLED
  page_data.value.pointTo(memory.potentiometers_enabled);
  page_data.description = "Enable/disable knobs";
#endif

  return true;
}

bool menuSelectSaveMemory() {
  saveMemory();

  return true;
}

bool menuSelectDefaultMemory() {
  defaultMemory();

  return true;
}
#endif