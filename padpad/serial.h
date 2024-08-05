struct Message {
  String message;
  byte key;
  String value;
};

void serialSend(String key, String value) {
  Serial.print(key + MESSAGE_SEP + value + MESSAGE_END);
}
