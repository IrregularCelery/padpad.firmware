#pragma once

/*--------------------------- General settings ---------------------------*/

#define DEVICE_NAME "PadPad"
#define DEVICE_MANUFACTURER "Irregular Celery"

#define PAIR_CHECK_INTERVAL 1000  // Interval of retrying (is ms)

// Make sure to change this if you edited `analogReadResolution(10)`
#define CUSTOM_ADC_RESOLUTION 1024  // 2^10

// Multi-core functionality
// If your microcontroller has more than one core, it's recommended
// to have this option on. Otherwise, make sure to set it to false.
// This will make sure all the functionalities are seemless and
// responsive, primarily for display updates.
#define MULTI_CORE_OPERATIONS true

#define SECONDS_PER_DAY 86400

/*---------------------------- Helper macros -----------------------------*/

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

/*------------------------- Components settings --------------------------*/
// You can disable any of these features if you don't need them

// NOT RECOMMENDED : you wouldn't be able to see the
// current state of the program if you disable this feature.
// e.g. PAIRING, CONNECTING, DISCONNECING, etc.
#define LED_DISABLED false

#define POTENTIOMETERS_DISABLED true

#define JOYSTICK_DISABLED false

#define ROTARY_ENCODER_DISABLED false

#define DISPLAY_DISABLED false

/*--------------------------- Profile settings ---------------------------*/

// changing a profile doesn't inherently change things here, other than
// telling the software, a profile change was happened.
// Also changing the current profile isn't available until the device
// and software are paired!

// You can have up to 256 different profiles. Profile 0 AKA profiles[0]
// is reserved for device's internal profile in which, keyboard buttons
// are read from its memory. You still can bind other interactions to
// keys in that profile, but you need to set it in the software and
// upload to device.

#define MULTIPLE_PROFILES true

/*--------------------------- Buttons settings ---------------------------*/

struct Layout {
  byte key;
  byte mod;
};

// Even though the buttons keymap is basically just a stream of numbers,
// to be able to have more button with less pins, we create a matrix
#define ROWS 3
#define COLS 5

// Pins of the button matrix
byte ROW_PINS[ROWS] = { 0, 1, 2 };
byte COL_PINS[COLS] = { 3, 4, 5, 6, 7 };

// Check out `buttons_layout` in `default_memory` for buttons default layout

#define DEBOUNCE_TIME 10

/*---------------------------- Modkey settings ---------------------------*/

// you can set one of your buttons to act as a key modifier, which means
// for example if you set a button as the letter "A", now if you hold the modkey
// it should be some other thing that you've set in the program.
// NOTE: any key with the character `255` assigned to its key, is the modkey.
// You can change your keys and their asinged letter from the application
// check out the software: https://github.com/IrregularCelery/padpad.software
#define MODKEY_DISABLED false

/*-------------------------- RGB LED settings ----------------------------*/

// WS2812B module
#if !LED_DISABLED
#define LED_PIN 8
#endif

/*----------------------- Potentiometers settings -----------------------*/

#if !POTENTIOMETERS_DISABLED

// If you're not using an analog multiplexer like CD4051BE, set this to true,
// and change `potentiometer_pins` for your need
#define ANALOG_MULTIPLEXER_DISABLED false

#if !ANALOG_MULTIPLEXER_DISABLED
// In case of having an analog multiplexer, these pins
// represent channel selector pins on the IC (A, B, C) *DIGITAL PINS*
const int selector_pins[] = { 16, 17, 18 };

// Maximum amount of potentiometers for an 8-channel multiplexer is, well... 8!
// but, since we might not use all of them, we need to set the count manually
// MAKE SURE TO GROUND REST OF THE UNUSED CHANNELS TO REDUCE CROSS-TALK AND NOISE!
const int potentiometers_count = 2;

// Common pin of analog multiplexer IC *ANALOG PIN*
const int multiplexer_analog_common = A0;
#else
// If you don't want to use an analog multiplexer,
// and/or want to use your microcontroller's ADC pins,
// make sure to set `ANALOG_MULTIPLEXER_DISABLED` to true
// and set these pins. Here these pins are *ANALOG PINS*
const int potentiometer_pins[] = { A0, A1, A2 };
const int potentiometers_count = sizeof(potentiometer_pins) / sizeof(potentiometer_pins[0]);
#endif
#else  // POTENTIOMETERS_DISABLED
#define ANALOG_MULTIPLEXER_DISABLED true
#endif

/*-------------------------- Joystick settings ---------------------------*/

// Joystick pins
#if !JOYSTICK_DISABLED
#define JOYSTICK_PIN_X A1  // X-axis analog pin
#define JOYSTICK_PIN_Y A2  // Y-axis analog pin
#define JOYSTICK_PIN_BUTTON 19

// Small number so, the analog noise wouldn't trigger joystick movement
// because of `CUSTOM_ADC_RESOLUTION`, the adc values are between 0-1023
#define JOYSTICK_DEADZONE 50
// Minimum time between joystick triggers, for example for mouse move,
// every 20 ms, a step of value that was accumulated in the past cycles
// will be counted instead of every cycle. This ensures even the small-
// values won't be ignored. Like for mouse movement, even if the joystick-
// movement is small, it can't go below 1px of cursor movement, so,
// the result, would still be faster that intended.
// ... BASICALLY A TIME-BASED THROTTLING *sigh*
#define JOYSTICK_TRIGGER_INTERVAL 20
#endif

/*----------------------- Rotary encoder settings ------------------------*/

#if !ROTARY_ENCODER_DISABLED
// If the rotation was inverse, switch PIN1 and PIN2 (CLK OR DT)
#define ROTARY_ENCODER_PIN1 20
#define ROTARY_ENCODER_PIN2 21
#define ROTARY_ENCODER_BUTTON 22

#define ROTARY_ENCODER_TRIGGER_DELAY 100  // Slight delay time to avoid \
                                          // infinite acceleration
#endif

/*--------------------------- Display settings ---------------------------*/

// We're using ST7920 GLCD
#if !DISPLAY_DISABLED

// Menu Setting
enum ViewType {  // Types of views that can be shown on the display
  VIEW_HOME,     // Home view to show a logo or information
  VIEW_MENU,     // Menu view
  VIEW_PAGE,     // Page view to display/change values
};

// Make sure to check `menus.h` for examples
struct MenuItem {
  const char* title;
  const uint8_t* icon = default_icon;  // X BitMap icon
  bool (*callback)() = nullptr;        // Function that'd be called when entering menu item
  MenuItem* sub_menu = nullptr;        // Set to nullptr if this item doesn't have a submenu
  int sub_menu_size = 0;               // Size of the sub_menu or 0 if doesn't have one
};

struct Menu {
  MenuItem* items;
  int size;
  int index;
  int offset;
  Menu* last_menu;
};

struct PageData {
  DynamicRef value;
  String title = "";
  String description = "";  // Maximum 24 characters per line for DISPLAY_WITH 128 (23 for goToMessage())
};

// Pins
#define DISPLAY_RS_PIN 13  // CS
#define DISPLAY_RW_PIN 11  // TX/MOSI
#define DISPLAY_E_PIN 10   // SCK
#define DISPLAY_RST_PIN 9  // Any digital pin

// General
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_PADDING 4

#define DISPLAY_DEFAULT_FONT u8g2_font_6x10_tr
#define DISPLAY_CONFIRM_BUTTON_TEXT "OK"
#define DISPLAY_TRUE_BUTTON_TEXT "Enable"
#define DISPLAY_FALSE_BUTTON_TEXT "Disable"

#define DISPLAY_DEFAULT_VIEW_TIMEOUT 3000  // Time before going back to `Home` view (in ms)
#define DISPLAY_AUTO_UPDATE_INTERVAL 1000  // (in ms)

// Menu defaults
#define MENU_MAX_VISIBLE_ITEMS 5
#define MENU_ITEM_FRAME_HEIGHT 11
#define MENU_ITEM_FRAME_RADIUS 4
#endif

/*--------------------------- Memory settings ----------------------------*/

struct Memory {
  uint16_t sector_check;

  Layout buttons_layout[ROWS * COLS];

  bool keyboard_enabled;

#if !POTENTIOMETERS_DISABLED
  bool potentiometers_enabled;
#endif

#if !JOYSTICK_DISABLED
  bool joystick_mouse_enabled;
  float joystick_sensitivity;
#endif
};

#define FLASH_SIZE (2 * 1024 * 1024)                             // Flash size of the microcontroller e.g. RPI Pico: 2 MiB
#define FLASH_END_OFFSET (FLASH_SIZE - (FLASH_SECTOR_SIZE * 2))  // Reserve the last sector of flash for saving memory
#define SECTOR_CHECK 51690                                       // A *totally* random number to check if the config memory exists on the flash

// Will only be used on code uploading, and/or the operation of reading
// the flash was unsuccessful. WILL BE IGNORED IF CONFIG MEMORY ISN'T EMPTY!
// Config memory can be changed/set from the software
Memory default_memory = {
  .sector_check = SECTOR_CHECK,

  // Buttons layout currently only keeps one letter to act as keyboard input
  // Number 255 and 0 are reserved. 255 for modkey and 0 for nothing.
  // Keep in mind 0 and '0' are different! 0 is NULL and '0' is character '0' which is number 48
  .buttons_layout = {
    { 0, 'O' },    // Button 1
    { 'B', 'P' },  // Button 2
    { 'C', 'Q' },  // Button 3
    { 'D', 'R' },  // ...
    { 'E', 'S' },
    { 'F', 'T' },
    { 'G', 'U' },
    { 'H', 'V' },
    { 'I', 'W' },
    { 'J', 'X' },
    { 'K', 'Y' },
    { 'L', 'Z' },  // ...
    { 'M', '0' },  // Button 13
    { 'N', '1' },  // Button 14
    { 255, 0 },    // Button 15
  },

  .keyboard_enabled = true,

#if !POTENTIOMETERS_DISABLED
  .potentiometers_enabled = true,
#endif

#if !JOYSTICK_DISABLED
  .joystick_mouse_enabled = true,
  .joystick_sensitivity = 0.5f,
#endif
};

/*--------------------------- Serial settings ----------------------------*/

// If you want to change these settings, make sure to set the same in the
// software as well. otherwise the connection would fail!
#define BAUD_RATE 38400
#define MESSAGE_SEP ":"
#define MESSAGE_END ";"
#define MESSAGE_SEP_INNER "|"

/*----------------------------- Debug settings ---------------------------*/

// Most of these toggles are only for indevisual component testing, and will ignore
// the rest of the program execution. Only one can be set to true at a time, otherwise
// the first one in order, will be running.
// MAKE SURE TO SET THEM TO FALSE IN THE END OF TESTING!

// If this is true, the memory will be reset to default on startup
#define DEBUG_RESET_MEMORY false

// This is only for testing the buttons and getting their IDs and stuff,
// if this is true, the rest of the functionalities are disabled!
#define DEBUG_BUTTONS false

// This is only for testing the potentiometers and getting their values,
// if this is true, the rest of the functionalities are disabled!
#define DEBUG_POTENTIOMETERS false

// This is only for testing the rotary encoder and getting its values,
// if this is true, the rest of the functionalities are disabled!
#define DEBUG_ROTARY_ENCODER false

// This is only for testing the display and showing some debug text on it,
// if this is true, you're still able to use the rest of the functionalities!
#define DEBUG_DISPLAY false

/*------------------------------- Warnings -------------------------------*/

#if DISPLAY_DISABLED
#if !ROTARY_ENCODER_DISABLED
#warning RotaryEncoder is useless without a display!
#endif
#endif

#if ROTARY_ENCODER_DISABLED
#if !DISPLAY_DISABLED
#warning You can't move between Display's menu items without a RotaryEncoder!
#endif
#endif