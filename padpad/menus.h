// Forward declaration
void menuBack();

MenuItem memory_to_default_submenu[] = {
  {
    .title = "No",
    .icon = back_icon,
    .callback = menuBack,
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
    .callback = menuBack,
  },
};

MenuItem main_menu[] = {
  {
    .title = "Mouse",
    .icon = mouse_icon,
  },
  {
    .title = "Keyboard",
    .icon = keyboard_icon,
  },
  {
    .title = "Potentiometer",
    .icon = potentiometer_icon,
  },
  {
    .title = "Memory",
    .icon = memory_icon,
    .sub_menu = memory_submenu,
    .sub_menu_size = ARRAY_SIZE(memory_submenu),
  },
  {
    .title = "Exit",
    .icon = back_icon,
    .callback = menuBack,
  },
};
