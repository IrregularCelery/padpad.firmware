#define LED_PIN 10

/*--------------------------- Serial settings ---------------------------*/

#define BAUD_RATE 38400
#define MESSAGE_SEP ":"
#define MESSAGE_END ";"

/*--------------------------- Buttons settings ---------------------------*/

// Even though the buttons keymap is basically just a stream of numbers,
// to be able to have more button with less pins, we create a matrix
#define ROWS 3
#define COLS 5

// Pins of the button matrix
const byte ROW_PINS[ROWS] = { 2, 3, 4 };
const byte COL_PINS[COLS] = { 5, 6, 7, 8, 9 };

/*--------------------------- Modkey settings ---------------------------*/

// you can set one of your buttons to act as a key modifier, which means
// for example if you set a button as the letter "A", now if you hold the modkey
// it should be some other thing that you've set in the program.
// NOTE: any key with the character `255` asigned to, is the modkey.
// You can change your keys and their asinged letter from the application
// check out the software: https://github.com/IrregularCelery/padpad.software
#define MODKEY_DISABLED false

/*--------------------------- General settings ---------------------------*/

#define DEBOUNCE_TIME 10

/*--------------------------- Debug settings ---------------------------*/

// Make sure to set this to false, this is only for testing the buttons and getting
// their IDs and stuff, if this is true, the rest of the functionalities are disabled!
#define DEBUG_BUTTONS false
