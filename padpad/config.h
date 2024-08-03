#define LED_PIN 10

// Serial settings
#define BAUD_RATE 38400
#define MESSAGE_SEP ":"
#define MESSAGE_END ";"

// Buttons settings
// Even though the buttons keymap is basically just a stream of numbers,
// to be able to have more button with less pins, we create a matrix
#define ROWS 3
#define COLS 5

// Pins of the button matrix
const byte ROW_PINS[ROWS] = { 2, 3, 4 };
const byte COL_PINS[COLS] = { 5, 6, 7, 8, 9 };

// General settings
#define DEBOUNCE_TIME 10

// Debugging
#define DEBUG_BUTTONS true