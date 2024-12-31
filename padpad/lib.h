#pragma once

class Button {
public:
  Button(uint8_t pin, unsigned long double_click_timeout = 300, unsigned long debounce_timeout = 10) {
    pin_ = pin;
    double_click_timeout_ = double_click_timeout;
    debounce_timeout_ = debounce_timeout;

    pinMode(pin_, INPUT_PULLUP);

    last_state_ = digitalRead(pin_);
    state_ = last_state_;
  }

  void tick() {
    unsigned long current_time = millis();
    bool current_state = digitalRead(pin_);

    // Handle debouncing
    if (current_state != last_state_) {
      last_debounce_time_ = current_time;
    }

    if ((current_time - last_debounce_time_) > debounce_timeout_) {
      if (current_state != state_) {
        state_ = current_state;

        // Button was pressed
        if (state_ == LOW) {
          handlePress(current_time);
        }

        // Button was released
        if (state_ == HIGH) {
          release_detected_ = true;
        }
      }
    }

    last_state_ = current_state;

    // Handle double-click timeout
    if (ready_for_double_click_ && ((current_time - last_press_time_) > double_click_timeout_)) {
      ready_for_double_click_ = false;
      click_count_ = 0;
    }
  }

  bool pressed() {
    if (pressed_detected_) {
      pressed_detected_ = false;

      return true;
    }

    return false;
  }

  bool released() {
    if (release_detected_) {
      release_detected_ = false;

      return true;
    }

    return false;
  }

  bool doubleClicked() {
    if (double_click_detected_) {
      double_click_detected_ = false;

      return true;
    }

    return false;
  }

private:
  void handlePress(unsigned long current_time) {
    if (ready_for_double_click_) {
      click_count_++;

      if (click_count_ == 2) {
        double_click_detected_ = true;
        ready_for_double_click_ = false;
        click_count_ = 0;
      }
    } else {
      pressed_detected_ = true;
      ready_for_double_click_ = true;
      click_count_ = 1;
      last_press_time_ = current_time;
    }
  }

  uint8_t pin_;
  unsigned long debounce_timeout_;
  unsigned long double_click_timeout_;
  unsigned long last_debounce_time_ = 0;
  unsigned long last_press_time_ = 0;

  bool last_state_;
  bool state_;

  bool ready_for_double_click_ = false;
  uint8_t click_count_ = 0;

  bool pressed_detected_ = false;
  bool release_detected_ = false;
  bool double_click_detected_ = false;
};

class DynamicRef {
public:
  enum VariableType {
    None,
    Bool,
    Int,
    Float,
  };

  DynamicRef() = default;

  void pointTo(bool& value) {
    _value = &value;
    _type = Bool;
  }
  void pointTo(int& value) {
    _value = &value;
    _type = Int;
  }
  void pointTo(float& value) {
    _value = &value;
    _type = Float;
  }

  void reset() {
    _value = nullptr;
    _type = None;
  }

  VariableType getType() {
    return _type;
  }

  // Bool
  void operator=(bool value) {
    if (_type == Bool) {
      *static_cast<bool*>(_value) = value;
    }
  }
  bool operator==(bool value) const {
    if (_type == Bool) {
      return *static_cast<bool*>(_value) == value;
    }

    return false;
  }
  bool operator!() const {
    if (_type == Bool) {
      return !(*static_cast<bool*>(_value));
    }

    return false;  // Return false for non-bool types
  }
  operator bool() const {
    if (_type == Bool) {
      return *static_cast<bool*>(_value);
    }

    return false;
  }

  // Int
  void operator=(int value) {
    if (_type == Int) {
      *static_cast<int*>(_value) = value;
    }
  }
  bool operator==(int value) const {
    if (_type == Int) {
      return *static_cast<int*>(_value) == value;
    }

    return false;
  }
  operator int() const {
    if (_type == Int) {
      return *static_cast<int*>(_value);
    }

    return 0;
  }
  void operator+=(int value) {
    switch (_type) {
      case Int:
        *static_cast<int*>(_value) += value;

        break;

      case Float:
        *static_cast<float*>(_value) += value;

        break;

      default:
        break;
    }
  }
  void operator-=(int value) {
    switch (_type) {
      case Int:
        *static_cast<int*>(_value) -= value;

        break;

      case Float:
        *static_cast<float*>(_value) -= value;

        break;

      default:
        break;
    }
  }

  // Float
  void operator=(float value) {
    if (_type == Float) {
      *static_cast<float*>(_value) = value;
    }
  }
  bool operator==(float value) const {
    if (_type == Float) {
      return *static_cast<float*>(_value) == value;
    }

    return false;
  }
  operator float() const {
    if (_type == Float) {
      return *static_cast<float*>(_value);
    }

    return 0.0f;
  }
  void operator+=(float value) {
    switch (_type) {
      case Float:
        *static_cast<float*>(_value) += value;

        break;

      case Int:
        *static_cast<int*>(_value) += (int)value;

        break;

      default:
        break;
    }
  }
  void operator-=(float value) {
    switch (_type) {
      case Float:
        *static_cast<float*>(_value) -= value;

        break;

      case Int:
        *static_cast<int*>(_value) -= (int)value;

        break;

      default:
        break;
    }
  }

private:
  void* _value = nullptr;
  VariableType _type = None;
};
