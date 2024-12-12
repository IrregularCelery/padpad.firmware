#pragma once

char* generateKeymap(byte row_count, byte col_count) {
  int size = row_count * col_count;
  char* keymap = new char[size];
  char start = 1;  // 1(number) and not '1'(char)

  for (int i = 0; i < size; i++)
    keymap[i] = start + i;

  return keymap;
}

// Function to smooth out analog reading and mapping it to a 100 step value (0-99)
int analogStepRead(int pin, int* last_step) {
  const float resolution = 1024;
  const float steps_count = 100;
  const float tolerance = 16;

  float value = analogRead(pin);

  // If last_step hasn't been initialized, start in the middle
  if (*last_step < 0 || *last_step >= steps_count) {
    *last_step = steps_count / 2;
  }

  // Create hysteresis boundaries
  float step_size = resolution / steps_count;
  float lower_bound = (*last_step * step_size) - (tolerance / 2);
  float upper_bound = (*last_step * step_size) + step_size + (tolerance / 2);

  // Calculate the current potential step
  float current_step = value / step_size;

  // If the current value is within the hysteresis zone, keep the last step
  if (value >= lower_bound && value <= upper_bound) {
    return *last_step;
  }

  *last_step = current_step;

  return current_step;
}

#if !ANALOG_MULTIPLEXER_DISABLED
// Function to select a specific channel on 8-bit multiplexer ICs, Here we use CD4051BE
void selectChannel(int channel) {
  // Ensure the channel is within valid range (0-7)
  channel = constrain(channel, 0, 7);

  // Set the select pins based on the binary representation of the channel
  for (int i = 0; i < 3 /* 3 bits = 8 channels */; i++) {
    digitalWrite(selector_pins[i], bitRead(channel, i));
  }
}
#endif
