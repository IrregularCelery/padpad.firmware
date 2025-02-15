# PadPad Firmware

#### PadPad Firmware is designed to run on the Raspberry Pi Pico (or other microcontrollers with minor modifications) and provides core functionality for handling button inputs, potentiometers, and various other components. It communicates with the PadPad software via Serial over USB while also supporting HID keyboard press in "Button Memory" mode.

### [🚀 Software Repository](https://github.com/IrregularCelery/padpad.software) | [🔧 Schematic (Coming Soon!)]()

## Table of Contents

- [Overview](#-overview)
- [Setup & Requirements](#%EF%B8%8F-setup--requirements)
  - [Prerequisites](#prerequisites)
  - [Uploading](#uploading)
  - [Porting to Other Boards](#porting-to-other-boards)
- [Configuration](#%EF%B8%8F-configuration-via-configh)
  - [Components Settings](#components-settings)
  - [RGB LED](#rgb-led)
  - [Joystick](#joystick)
  - [Rotary Encoder](#rotary-encoder)
  - [Display](#display-st7920-graphical-lcd)
  - [Memory & Data Storage](#memory--data-storage)
  - [Serial Communication](#serial-communication)
- [Images](#%EF%B8%8F-images)
- [Contributing](#-contributing)
- [Links](#-links)
- [License](#-license)

## 📖 Overview

**This firmware powers the PadPad device, enabling:**

- **Multi-component support**: Buttons, potentiometers, joysticks, rotary encoders, and displays.
- **HID Keyboard Mode**: Standalone "`Button Memory`" functionality (no software required!).
- **Customizable via `config.h`**: Enable/disable components, adjust pins, and fine-tune behavior.
- **Cross-board compatibility**: Primarily tested on Raspberry Pi Pico, but adaptable to others.

## ⚙️ Setup & Requirements

### Prerequisites

- **Arduino IDE** with [Raspberry Pi Pico support](https://github.com/earlephilhower/arduino-pico).
- Set **USB Stack** to _Adafruit TinyUSB_ (Tools > USB Stack).

### Uploading

1. Clone the repository:

   ```bash
   git clone https://github.com/IrregularCelery/padpad.firmware.git
   ```

2. Open `padpad.ino` in Arduino IDE.
3. Configure `config.h` (see below).
4. Upload to your device!

### Porting to Other Boards

The firmware is written to be **board-agnostic**, meaning it should work on most microcontrollers. However,<br>**multi-core operations** are exclusive to the RPI Pico and must be disabled on other boards by setting:

```cpp
#define MULTI_CORE_OPERATIONS false
```

> - With some adjustments, the **multi-core functionality** should work with other boards as well.
> - The primary benefit of **multi-core operations** is improved display responsiveness.<br> _Display updates are handled on the second core._

## 🛠️ Configuration (via `config.h`)

### Components Settings

Each component in the firmware is individually configurable and can be enabled/disabled from `config.h`. The only required component is **Buttons**.

Enable/disable components by setting `[COMPONENT]_DISABLED` to `true`:

```cpp
// Example: Disable the joystick
#define JOYSTICK_DISABLED true
```

| Component          | Key Features                                                 |
| ------------------ | ------------------------------------------------------------ |
| **Buttons**        | Matrix layout with anti-ghosting diodes.                     |
| **RGB LED**        | WS2812B support for connection status/effects.               |
| **Potentiometers** | Supports analog pins or multiplexers (e.g., CD4051BE).       |
| **Joystick**       | HID mouse movement simulation + Adjustanble deadzone.        |
| **Rotary Encoder** | Requires a display for viewing menus.                        |
| **Display**        | ST7920 GLCD (SPI). Requires a rotary encoder for navigating. |

### Buttons (Matrix Layout)

PadPad buttons are laid out in a matrix configuration to optimize GPIO usage.

Example configuration for a 3x5 button matrix:

```cpp
#define ROWS 3
#define COLS 5

byte ROW_PINS[ROWS] = { 0, 1, 2 };
byte COL_PINS[COLS] = { 3, 4, 5, 6, 7 };
```

_(TODO: Add schematic for diode-based ghosting prevention.)_

### RGB LED

An optional WS2812B RGB LED (or compatible module) can be used for status indication. The LED will signal when the device is connected or disconnected and has an RGB rainbow effect as a fun bonus.

Example configuration:

```cpp
#define LED_PIN 8
```

### Potentiometers

Potentiometers can be connected either directly to analog pins or through an analog multiplexer (e.g., **CD4051BE**).

Enable/disable multiplexer usage:

```cpp
#define ANALOG_MULTIPLEXER_DISABLED false
```

More details are in the **"Potentiometers Settings"** section of `config.h`.

### Joystick

Joysticks require two analog pins for X/Y axes and one digital pin for the button.

Example configuration:

```cpp
#define JOYSTICK_PIN_X A1
#define JOYSTICK_PIN_Y A2
#define JOYSTICK_PIN_BUTTON 19

#define JOYSTICK_DEADZONE 50
```

_(Multiplexer support for the joystick is not implemented.)_

### Rotary Encoder

The Rotary Encoder is used for navigating menus on the display. It is only useful if a display is enabled.

Example configuration:

```cpp
#define ROTARY_ENCODER_PIN1 20
#define ROTARY_ENCODER_PIN2 21
#define ROTARY_ENCODER_BUTTON 22
```

- **Rotate Right** → Move down in the menu.
- **Rotate Left** → Move up in the menu.
- **Click Button** → Select menu option.

### Display (ST7920 Graphical LCD)

The display shows menu navigation, profile selection, and status information.

- The **ST7920 LCD** runs in software SPI mode because the RPI Pico operates at 3.3V, while the ST7920 needs 5V.
- Using a **TXS0108E** level converter allows hardware SPI mode, but software SPI is the default.

More details are in the **"Display Settings"** section of `config.h`.

### Memory & Data Storage

The firmware saves settings in flash memory, allowing persistence between restarts.

- **Settings are NOT saved automatically** to avoid excessive flash writes.
- Navigate to the **Memory menu** on the display to save settings manually.

Example button layout stored in memory:

```cpp
.buttons_layout = {
//{ key, mod },
  {  0 , 'O' },  // Button 1
  { 'B', 'P' },  // Button 2
  { 'C', 'Q' },  // Button 3
  { 'D', 'R' },  // ...
  { 'E', 'S' },
  { 'F', 'T' },
  { 'G', 'U' },
  { 'H', 'V' },
  { 'I', 'W' },
  { 'J', 'X' },
  { 'K', 'Y' },
  { 'L', 'Z' },  // ...
  { 'M', '0' },  // Button 13
  { 'N', '1' },  // Button 14
  { 255,  0  },  // Button 15
},
```

- The first character, `key`, represents the keyboard key pressed when the button is activated.
- The second character, `mod`, represents the key pressed when the button is activated _while_ the `ModKey` is held.
- `255` and `0` are reserved (e.g., `255` for `ModKey`, `0` for no action).

### Serial Communication

Default communication format:

```cpp
#define MESSAGE_SEP ":"
#define MESSAGE_END ";"
#define MESSAGE_SEP_INNER "|"
```

> [!CAUTION]
> If modified, the software settings must be updated accordingly!

## 🖼️ Images

<details>
<summary>Show images</summary>

_(TODO: Add images of the device.)_

</details>

## 🤝 Contributing

Contributions are welcome! If you find issues or have feature requests, open an issue or pull request.

## 🔗 Links

- [Software Setup Guide](https://github.com/IrregularCelery/padpad.software)
- [Report a Bug](https://github.com/IrregularCelery/padpad.firmware/issues)
- [YouTube Video (not a tutorial)]()

## 📜 License

**PadPad firmware** is 100% free and open-source under the [MIT License](/LICENSE).
