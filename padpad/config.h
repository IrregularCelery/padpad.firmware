/*--------------------------- General settings ---------------------------*/

#define DEVICE_NAME "PadPad"
#define DEVICE_MANUFACTURER "Irregular Celery"

// You can disable any of these features if you don't need them
#define LED_DISABLED false  // NOT RECOMMENDED : you wouldn't be able to \
                                                  // see the current state of the program if you \
                                                  // disable this feature. \
                                                  // e.g. PAIRING, CONNECTING, DISCONNECING, etc.
#define JOYSTICK_DISABLED false
#define ROTARY_ENCODER_DISABLED false

/*---------------------------- Pins settings -----------------------------*/

// WS2812B module
#if !LED_DISABLED
#define LED_PIN 10
#endif

#if !JOYSTICK_DISABLED
#define JOYSTICK_PIN_X 8
#define JOYSTICK_PIN_Y 9
#endif

#if !ROTARY_ENCODER_DISABLED
#define ROTARY_ENCODER_PIN1 11
#define ROTARY_ENCODER_PIN2 12
#define ROTARY_ENCODER_BUTTON 13
#endif

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

// Default buttons layout. WOULD BE IGNORED IF THE EEPROM ISN'T EMPTY
// Number 255 and 0 are reserved. 255 for modkey and 0 for nothing.
// Keep in mind 0 and '0' are different! 0 is NULL and '0' is character '0' which is number 48
Layout layout[ROWS * COLS] = {
  { 0, 'o' },  { 'b', 'p' },  { 'c', 'q' },  { 'd', 'r' },  { 'e', 's' },
  { 'f', 't' },  { 'g', 'u' },  { 'h', 'v' },  { 'i', 'w' },  { 'j', 'x' },
  { 'k', 'y' },  { 'l', 'z' },  { 'm', '0' },  { 'n', '1' },  { 255, 0 },
};

#define DEBOUNCE_TIME 10

/*---------------------------- Serial settings ----------------------------*/

#define BAUD_RATE 38400
#define MESSAGE_SEP ":"
#define MESSAGE_END ";"

/*---------------------------- Modkey settings ---------------------------*/

// you can set one of your buttons to act as a key modifier, which means
// for example if you set a button as the letter "A", now if you hold the modkey
// it should be some other thing that you've set in the program.
// NOTE: any key with the character `255` assigned to, is the modkey.
// You can change your keys and their asinged letter from the application
// check out the software: https://github.com/IrregularCelery/padpad.software
#define MODKEY_DISABLED false

/*----------------------------- Debug settings ---------------------------*/

// Make sure to set this to false, this is only for testing the buttons and getting
// their IDs and stuff, if this is true, the rest of the functionalities are disabled!
#define DEBUG_BUTTONS false
