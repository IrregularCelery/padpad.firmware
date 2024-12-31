#include "config.h"
#if !DISPLAY_DISABLED
// Forward declaration
bool menuGoBack();
bool menuSelectMouse();
bool menuSelectKeyboard();
bool menuSelectPotentiometers();
bool menuSelectSaveMemory();

MenuItem memory_to_default_submenu[] = {
  {
    .title = "No",
    .icon = back_icon,
    .callback = menuGoBack,
  },
  {
    .title = "Yes",
    .icon = default_icon,
    .callback = menuSelectSaveMemory,
  },
};

MenuItem memory_submenu[] = {
  {
    .title = "Default settings",
    .icon = memory_icon,
    .sub_menu = memory_to_default_submenu,
    .sub_menu_size = ARRAY_SIZE(memory_to_default_submenu),
  },
  {
    .title = "Save settings",
    .icon = memory_icon,
  },
  {
    .title = "Back",
    .icon = back_icon,
    .callback = menuGoBack,
  },
};

MenuItem main_menu[] = {
#if !JOYSTICK_DISABLED
  {
    .title = "Mouse",
    .icon = mouse_icon,
    .callback = menuSelectMouse,
  },
#endif
  {
    .title = "Keyboard",
    .icon = keyboard_icon,
    .callback = menuSelectKeyboard,
  },
#if !POTENTIOMETERS_DISABLED
  {
    .title = "Potentiometer",
    .icon = potentiometer_icon,
    .callback = menuSelectPotentiometers,
  },
#endif
  {
    .title = "Memory",
    .icon = memory_icon,
    .sub_menu = memory_submenu,
    .sub_menu_size = ARRAY_SIZE(memory_submenu),
  },
  {
    .title = "Exit",
    .icon = back_icon,
    .callback = menuGoBack,
  },
};
#endif
