#pragma once

#include "config.h"
#if !DISPLAY_DISABLED
// Forward declaration
bool menuSelectBack();
bool menuSelectMouseSensitivity();
bool menuSelectMouseToggle();
bool menuSelectKeyboard();
bool menuSelectPotentiometers();
bool menuSelectSaveMemory();
bool menuSelectDefaultMemory();

MenuItem default_memory_submenu[] = {
  {
    .title = "No",
    .icon = x_mark_icon,
    .callback = menuSelectBack,
  },
  {
    .title = "Yes",
    .icon = check_mark_icon,
    .callback = menuSelectDefaultMemory,
  },
};

MenuItem memory_submenu[] = {
  {
    .title = "Reset settings",
    .icon = reset_icon,
    .sub_menu = default_memory_submenu,
    .sub_menu_size = ARRAY_SIZE(default_memory_submenu),
  },
  {
    .title = "Save settings",
    .icon = upload_icon,
    .callback = menuSelectSaveMemory,
  },
  {
    .title = "Back",
    .icon = back_icon,
    .callback = menuSelectBack,
  },
};

MenuItem mouse_submenu[] = {
  {
    .title = "Sensitivity",
    .icon = cursor_acceleration_icon,
    .callback = menuSelectMouseSensitivity,
  },
  {
    .title = "Turn on/off",
    .icon = toggle_icon,
    .callback = menuSelectMouseToggle,
  },
  {
    .title = "Back",
    .icon = back_icon,
    .callback = menuSelectBack,
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
    .callback = menuSelectBack,
  },
};

// This menu will be filled dynamically
MenuItem* profiles_menu = nullptr;
#endif
