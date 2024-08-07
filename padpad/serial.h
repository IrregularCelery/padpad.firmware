struct Message {
  String message;
  byte key;
  String value;
};

void serialSend(String key, String value) {
  Serial.print(key + MESSAGE_SEP + value + MESSAGE_END);
  Serial.flush();
}

// Button states: 0 => RELEASE, 1 => PRESSED, 2 => HOLD
void serialSendButton(byte id, bool modkey, byte state) {
  String modkey_state = "m";

  if (modkey)
    modkey_state = "M";

  serialSend("b" + modkey_state + (String)id, (String)state);
}