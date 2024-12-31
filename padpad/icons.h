#pragma once

#define ICON_WIDTH 17
#define ICON_HEIGHT 16

const uint8_t default_icon[] = {
  0x00, 0x00, 0xfe, 0x06, 0xc0, 0xfe, 0xf2, 0x9f, 0xfe, 0x58, 0x35, 0xfe,
  0xac, 0x6a, 0xfe, 0x54, 0x55, 0xfe, 0xac, 0x6a, 0xfe, 0x54, 0x55, 0xfe,
  0xac, 0x6a, 0xfe, 0x54, 0x55, 0xfe, 0xac, 0x6a, 0xfe, 0x54, 0x55, 0xfe,
  0xa8, 0x2a, 0xfe, 0xf2, 0x9f, 0xfe, 0x06, 0xc0, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t arrow_up[] = {
  0x00, 0x01, 0xfe, 0x80, 0x03, 0xfe, 0xc0, 0x06, 0xfe, 0x60, 0x0c, 0xfe,
  0x30, 0x18, 0xfe, 0x18, 0x30, 0xfe, 0x08, 0x20, 0xfe, 0x04, 0x40, 0xfe,
  0x3c, 0x78, 0xfe, 0x20, 0x08, 0xfe, 0x20, 0x08, 0xfe, 0x20, 0x08, 0xfe,
  0x20, 0x08, 0xfe, 0xe0, 0x0f, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t arrow_up_filled[] = {
  0x00, 0x00, 0xfe, 0x00, 0x01, 0xfe, 0x80, 0x03, 0xfe, 0xc0, 0x07, 0xfe,
  0xe0, 0x0f, 0xfe, 0xf0, 0x1f, 0xfe, 0xf8, 0x3f, 0xfe, 0xfc, 0x7f, 0xfe,
  0xfc, 0x7f, 0xfe, 0xe0, 0x0f, 0xfe, 0xe0, 0x0f, 0xfe, 0xe0, 0x0f, 0xfe,
  0xe0, 0x0f, 0xfe, 0xe0, 0x0f, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t arrow_down[] = {
  0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0xe0, 0x0f, 0xfe, 0x20, 0x08, 0xfe,
  0x20, 0x08, 0xfe, 0x20, 0x08, 0xfe, 0x20, 0x08, 0xfe, 0x3c, 0x78, 0xfe,
  0x04, 0x40, 0xfe, 0x08, 0x20, 0xfe, 0x18, 0x30, 0xfe, 0x30, 0x18, 0xfe,
  0x60, 0x0c, 0xfe, 0xc0, 0x06, 0xfe, 0x80, 0x03, 0xfe, 0x00, 0x01, 0xfe
};

const uint8_t arrow_down_filled[] = {
  0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0xe0, 0x0f, 0xfe, 0xe0, 0x0f, 0xfe,
  0xe0, 0x0f, 0xfe, 0xe0, 0x0f, 0xfe, 0xe0, 0x0f, 0xfe, 0xfc, 0x7f, 0xfe,
  0xfc, 0x7f, 0xfe, 0xf8, 0x3f, 0xfe, 0xf0, 0x1f, 0xfe, 0xe0, 0x0f, 0xfe,
  0xc0, 0x07, 0xfe, 0x80, 0x03, 0xfe, 0x00, 0x01, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t back_icon[] = {
  0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x20, 0x00, 0xfe,
  0x30, 0x00, 0xfe, 0xe8, 0x1f, 0xfe, 0x04, 0x20, 0xfe, 0x02, 0x40, 0xfe,
  0x04, 0x40, 0xfe, 0xe8, 0x5f, 0xfe, 0x30, 0x50, 0xfe, 0x20, 0x28, 0xfe,
  0x00, 0x1c, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t x_mark_icon[] = {
  0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe,
  0x20, 0x08, 0xfe, 0x50, 0x14, 0xfe, 0xa0, 0x0a, 0xfe, 0x40, 0x05, 0xfe,
  0x80, 0x02, 0xfe, 0x40, 0x05, 0xfe, 0xa0, 0x0a, 0xfe, 0x50, 0x14, 0xfe,
  0x20, 0x08, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t check_mark_icon[] = {
  0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x20, 0xfe,
  0x00, 0x50, 0xfe, 0x00, 0x28, 0xfe, 0x10, 0x14, 0xfe, 0x28, 0x0a, 0xfe,
  0x50, 0x05, 0xfe, 0xa0, 0x02, 0xfe, 0x40, 0x01, 0xfe, 0x80, 0x00, 0xfe,
  0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t reset_icon[] = {
  0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0xf0, 0x1f, 0xfe, 0x08, 0x20, 0xfe,
  0xe8, 0x1f, 0xfe, 0x28, 0x00, 0xfe, 0x44, 0x10, 0xfe, 0x28, 0x28, 0xfe,
  0x10, 0x44, 0xfe, 0x00, 0x28, 0xfe, 0x00, 0x28, 0xfe, 0xf0, 0x2f, 0xfe,
  0x08, 0x20, 0xfe, 0xf0, 0x1f, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t toggle_icon[] = {
  0x00, 0x00, 0xfe, 0xf0, 0x1f, 0xfe, 0x08, 0x24, 0xfe, 0x04, 0x42, 0xfe,
  0x04, 0x42, 0xfe, 0x08, 0x24, 0xfe, 0xf0, 0x1f, 0xfe, 0x00, 0x00, 0xfe,
  0x00, 0x00, 0xfe, 0xf0, 0x1f, 0xfe, 0x48, 0x20, 0xfe, 0x84, 0x40, 0xfe,
  0x84, 0x40, 0xfe, 0x48, 0x20, 0xfe, 0xf0, 0x1f, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t upload_icon[] = {
  0x00, 0x00, 0xfe, 0x00, 0x01, 0xfe, 0x80, 0x02, 0xfe, 0x40, 0x04, 0xfe,
  0x80, 0x02, 0xfe, 0x80, 0x02, 0xfe, 0x80, 0x02, 0xfe, 0x80, 0x02, 0xfe,
  0x08, 0x21, 0xfe, 0x14, 0x50, 0xfe, 0xf4, 0x5f, 0xfe, 0x04, 0x40, 0xfe,
  0xf8, 0x3f, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t mouse_icon[] = {
  0xe0, 0x0f, 0xfe, 0x10, 0x11, 0xfe, 0x88, 0x22, 0xfe, 0x88, 0x22, 0xfe,
  0x88, 0x22, 0xfe, 0x88, 0x22, 0xfe, 0x18, 0x31, 0xfe, 0xe8, 0x2f, 0xfe,
  0x08, 0x20, 0xfe, 0x08, 0x20, 0xfe, 0x08, 0x20, 0xfe, 0x08, 0x20, 0xfe,
  0x08, 0x20, 0xfe, 0x08, 0x20, 0xfe, 0x10, 0x10, 0xfe, 0xe0, 0x0f, 0xfe
};

const uint8_t cursor_acceleration_icon[] = {
  0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x02, 0xfe, 0xf4, 0x06, 0xfe,
  0x00, 0x0a, 0xfe, 0xd0, 0x12, 0xfe, 0x00, 0x22, 0xfe, 0xe8, 0x42, 0xfe,
  0x00, 0x82, 0xfe, 0xfa, 0xe2, 0xfe, 0x00, 0x2a, 0xfe, 0x00, 0x56, 0xfe,
  0x00, 0x50, 0xfe, 0x00, 0x20, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t keyboard_icon[] = {
  0x00, 0x40, 0xfe, 0x00, 0x3e, 0xfe, 0x00, 0x01, 0xfe, 0xfe, 0xff, 0xfe,
  0x01, 0x00, 0xff, 0xad, 0x6a, 0xff, 0x01, 0x00, 0xff, 0x5d, 0x75, 0xff,
  0x01, 0x00, 0xff, 0xad, 0x6a, 0xff, 0x01, 0x00, 0xff, 0xf5, 0x5b, 0xff,
  0x01, 0x00, 0xff, 0xfe, 0xff, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t potentiometer_icon[] = {
  0x00, 0x01, 0xfe, 0x10, 0x10, 0xfe, 0xc0, 0x07, 0xfe, 0x70, 0x1c, 0xfe,
  0x1a, 0xb0, 0xfe, 0x08, 0x20, 0xfe, 0x0c, 0x60, 0xfe, 0x04, 0x40, 0xfe,
  0x7d, 0x40, 0xff, 0x04, 0x40, 0xfe, 0x0c, 0x60, 0xfe, 0x08, 0x20, 0xfe,
  0x1a, 0xb0, 0xfe, 0x70, 0x1c, 0xfe, 0xc0, 0x07, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t memory_icon[] = {
  0x00, 0x00, 0xfe, 0xf8, 0x3f, 0xfe, 0x54, 0x56, 0xfe, 0x52, 0x96, 0xfe,
  0x62, 0x90, 0xfe, 0xc2, 0x9f, 0xfe, 0x02, 0x80, 0xfe, 0x02, 0x80, 0xfe,
  0xe2, 0x8f, 0xfe, 0x12, 0x90, 0xfe, 0xd2, 0x97, 0xfe, 0x12, 0x90, 0xfe,
  0xd2, 0x97, 0xfe, 0x12, 0x90, 0xfe, 0xfc, 0x7f, 0xfe, 0x00, 0x00, 0xfe
};

const uint8_t settings_icon[] = {
  0x00, 0x00, 0xfe, 0xf0, 0x00, 0xfe, 0x9c, 0x7f, 0xfe, 0x9c, 0x7f, 0xfe,
  0xf0, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0x1e, 0xfe, 0xfc, 0x73, 0xfe,
  0xfc, 0x73, 0xfe, 0x00, 0x1e, 0xfe, 0x00, 0x00, 0xfe, 0xc0, 0x03, 0xfe,
  0x7c, 0x7e, 0xfe, 0x7c, 0x7e, 0xfe, 0xc0, 0x03, 0xfe, 0x00, 0x00, 0xfe
};
