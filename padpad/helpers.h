char* generateKeymap(byte row_count, byte col_count) {
  int size = row_count * col_count;
  char* keymap = new char[size];
  char start = 1;  // 1(number) and not '1'(char)

  for (int i = 0; i < size; i++)
    keymap[i] = start + i;

  return keymap;
}
