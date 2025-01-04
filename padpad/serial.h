#pragma once

struct Message {
  String message;
  byte key;
  String value;
};

// Avoid using `FORBIDDEN_CHARACTERS` in your messages!
// Check out `FORBIDDEN_CHARACTERS` in `config.h` file
void serialSend(String key, String value) {
  Serial.print(key + MESSAGE_SEP + value + MESSAGE_END);
  Serial.flush();
}

// Avoid using `FORBIDDEN_CHARACTERS` in your messages!
// Check out `FORBIDDEN_CHARACTERS` in `config.h` file
void serialError(String message) {
  serialSend("ERROR", message);
}

// Avoid using `FORBIDDEN_CHARACTERS` in your messages!
// Check out `FORBIDDEN_CHARACTERS` in `config.h` file
// For requesting software to change something, such as settings
void serialRequest(String key, String value = "") {
  serialSend("REQUEST", key + MESSAGE_SEP_INNER + value);
}

// Button states: 0 => RELEASE, 1 => PRESSED, 2 => HOLD (HOLD not implemented)
void serialSendButton(byte id, bool modkey, byte state) {
  String modkey_state = "m";

  if (modkey)
    modkey_state = "M";

  serialSend("b" + modkey_state + (String)id, (String)state);
}

// Potentiometer value is mapped to be 100 steps between 0 and 99
void serialSendPotentiometer(byte id, byte value) {
  // We add an empty character as the modkey So, the handling logic
  // inside the software wouldn't get too complicated.
  String modkey = "-";

  serialSend("p" + modkey + (String)(id + 1), (String)value);
}
