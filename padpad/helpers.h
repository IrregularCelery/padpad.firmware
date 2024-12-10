char* generateKeymap(byte row_count, byte col_count) {
  int size = row_count * col_count;
  char* keymap = new char[size];
  char start = 1;  // 1(number) and not '1'(char)

  for (int i = 0; i < size; i++)
    keymap[i] = start + i;

  return keymap;
}

int analogStepRead(int pin, int* last_step) {
  const int steps_count = 32;
  const int tolerance = 20;
  const int resolution = 1024;

  int value = analogRead(pin);
  
  // If last_step hasn't been initialized, start in the middle
  if (*last_step < 0 || *last_step >= steps_count) {
    *last_step = steps_count / 2;
  }
  
  // Create hysteresis boundaries
  int step_size = resolution / steps_count;
  int lower_bound = (*last_step * step_size) - (tolerance / 2);
  int upper_bound = (*last_step * step_size) + step_size + (tolerance / 2);

  // Calculate the current potential step
  int current_step = value / step_size;
  
  // If the current value is within the hysteresis zone, keep the last step
  if (value >= lower_bound && value <= upper_bound) {
    return *last_step;
  }
  
  *last_step = current_step;

  return current_step;
}