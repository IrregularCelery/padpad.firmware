#include "config.h"
#if !DISPLAY_DISABLED
// Forward declaration
bool menuGoBack();
bool menuSelectMouseSensitivity();
bool menuSelectMouseToggle();
bool menuSelectKeyboard();
bool menuSelectPotentiometers();
bool menuSelectSaveMemory();
bool menuSelectDefaultMemory();

MenuItem default_memory_submenu[] = {
  {
    .title = "No",
    .icon = back_icon,
    .callback = menuGoBack,
  },
  {
    .title = "Yes",
    .icon = default_icon,
  },
};

MenuItem memory_submenu[] = {
  {
    .title = "Default settings",
    .icon = memory_icon,
    .sub_menu = default_memory_submenu,
    .sub_menu_size = ARRAY_SIZE(default_memory_submenu),
  },
  {
    .title = "Save settings",
    .icon = memory_icon,
    .callback = menuSelectSaveMemory,
  },
  {
    .title = "Back",
    .icon = back_icon,
    .callback = menuGoBack,
  },
};

MenuItem mouse_submenu[] = {
  {
    .title = "Sensitivity",
    .icon = mouse_icon,
    .callback = menuSelectMouseSensitivity,
  },
  {
    .title = "Turn on/off",
    .icon = mouse_icon,
    .callback = menuSelectMouseToggle,
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
    .sub_menu = mouse_submenu,
    .sub_menu_size = ARRAY_SIZE(mouse_submenu),
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
