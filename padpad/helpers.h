#pragma once

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

char* generateKeymap(byte row_count, byte col_count) {
  int size = row_count * col_count;
  char* keymap = new char[size];
  char start = 1;  // 1(number) and not '1'(char)

  for (int i = 0; i < size; i++)
    keymap[i] = start + i;

  return keymap;
}

// Function to smooth out analog reading and mapping it to a 100 step value (0-99)
uint8_t analogStepRead(uint8_t pin, int* last_step) {
  const float steps_count = 100;
  const float tolerance = 16;

  float value = analogRead(pin);

  // If last_step hasn't been initialized, start in the middle
  if (*last_step < 0 || *last_step >= steps_count) {
    *last_step = steps_count / 2;
  }

  // Create hysteresis boundaries
  float step_size = CUSTOM_ADC_RESOLUTION / steps_count;
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

// ---------- Component related functions ---------- //

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

#if !ROTARY_ENCODER_DISABLED
bool _rotary_encoder_moved = false;

// If you're using an ESP board, this will be needed for the intrupt attached function
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

void IRAM_ATTR rotaryEncoderMove() {
  // Rotary encoder position change detected by intrupt pins
  _rotary_encoder_moved = true;
}

bool rotaryEncoderMoved() {
  return _rotary_encoder_moved;
}

// Algorithm and docs : https://www.pinteric.com/rotary.html
int8_t rotaryEncoderRead() {
  // Reset the position change
  _rotary_encoder_moved = false;

  static uint8_t lrmem = 3;
  static int lrsum = 0;
  static int8_t TRANS[] = { 0, -1, 1, 14, 1, 0, 14, -1, -1, 14, 0, 1, 14, 1, -1, 0 };

  // Read both pin states for rotation detection
  int8_t l = digitalRead(ROTARY_ENCODER_PIN1);
  int8_t r = digitalRead(ROTARY_ENCODER_PIN2);

  // Move previous value 2 bits to the left and add in our new values
  lrmem = ((lrmem & 0x03) << 2) + 2 * l + r;

  // Convert the bit pattern to a movement indicator (14 = impossible, ie switch bounce)
  lrsum += TRANS[lrmem];

  // Rotary encoder not in the neutral (detent) state
  if (lrsum % 4 != 0) {
    return 0;
  }

  // Rotary encoder in the neutral state - clockwise rotation
  if (lrsum == 4) {
    lrsum = 0;

    return 1;
  }

  // Rotary encoder in the neutral state - counterclockwise rotation
  if (lrsum == -4) {
    lrsum = 0;

    return -1;
  }

  // An impossible rotation has been detected - ignore the movement
  lrsum = 0;

  return 0;
}
#endif