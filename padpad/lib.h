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
    value_ = &value;
    type_ = Bool;
  }
  void pointTo(int& value) {
    value_ = &value;
    type_ = Int;
  }
  void pointTo(float& value) {
    value_ = &value;
    type_ = Float;
  }

  void reset() {
    value_ = nullptr;
    type_ = None;
  }

  VariableType getType() {
    return type_;
  }

  // Bool
  void operator=(bool value) {
    if (type_ == Bool) {
      *static_cast<bool*>(value_) = value;
    }
  }
  bool operator==(bool value) const {
    if (type_ == Bool) {
      return *static_cast<bool*>(value_) == value;
    }

    return false;
  }
  bool operator!() const {
    if (type_ == Bool) {
      return !(*static_cast<bool*>(value_));
    }

    return false;  // Return false for non-bool types
  }
  operator bool() const {
    if (type_ == Bool) {
      return *static_cast<bool*>(value_);
    }

    return false;
  }

  // Int
  void operator=(int value) {
    if (type_ == Int) {
      *static_cast<int*>(value_) = value;
    }
  }
  bool operator==(int value) const {
    if (type_ == Int) {
      return *static_cast<int*>(value_) == value;
    }

    return false;
  }
  operator int() const {
    if (type_ == Int) {
      return *static_cast<int*>(value_);
    }

    return 0;
  }
  void operator+=(int value) {
    switch (type_) {
      case Int:
        *static_cast<int*>(value_) += value;

        break;

      case Float:
        *static_cast<float*>(value_) += value;

        break;

      default:
        break;
    }
  }
  void operator-=(int value) {
    switch (type_) {
      case Int:
        *static_cast<int*>(value_) -= value;

        break;

      case Float:
        *static_cast<float*>(value_) -= value;

        break;

      default:
        break;
    }
  }

  // Float
  void operator=(float value) {
    if (type_ == Float) {
      *static_cast<float*>(value_) = value;
    }
  }
  bool operator==(float value) const {
    if (type_ == Float) {
      return *static_cast<float*>(value_) == value;
    }

    return false;
  }
  operator float() const {
    if (type_ == Float) {
      return *static_cast<float*>(value_);
    }

    return 0.0f;
  }
  void operator+=(float value) {
    switch (type_) {
      case Float:
        *static_cast<float*>(value_) += value;

        break;

      case Int:
        *static_cast<int*>(value_) += (int)value;

        break;

      default:
        break;
    }
  }
  void operator-=(float value) {
    switch (type_) {
      case Float:
        *static_cast<float*>(value_) -= value;

        break;

      case Int:
        *static_cast<int*>(value_) -= (int)value;

        break;

      default:
        break;
    }
  }

private:
  void* value_ = nullptr;
  VariableType type_ = None;
};

struct Color {
  uint8_t r, g, b;

  Color(uint8_t red, uint8_t green, uint8_t blue) {
    r = red;
    g = green;
    b = blue;
  }

  Color() {
    r = 0;
    g = 0;
    b = 0;
  }
};

class RGBController {
public:
  RGBController(unsigned long transition_duration = 1000 /* ms */) {
    start_color_ = RED;
    end_color_ = BLUE;
    duration_ = transition_duration;
    elapsed_time_ = 0;
    last_update_time_ = millis();
    paused_ = false;

    overridden_ = false;
    transitioning_from_override_ = false;
    transitioning_to_override_ = false;
    override_duration_ = 0;
    override_timeout_ = 0;

    flashing_ = false;
  }

  void pause() {
    paused_ = true;
  }

  void resume() {
    paused_ = false;

    last_update_time_ = millis();
  }

  // Parameter `delay` means the delay between each flashing (in ms)
  void flash(int count, Color color, unsigned long delay) {
    flash_color_ = color;
    current_flash_start_ = getCurrentColor();
    current_flash_end_ = flash_color_;

    flashing_ = true;

    flash_count_ = count * 2;  // Fading in and out
    current_flash_ = 0;

    single_flash_duration_ = delay;
    flash_elapsed_time_ = 0;
  }

  // Color of RGBController will be overridden until `timeout` runs out
  // `timeout` = 0 overrides indefinitely (in ms)
  void override(Color color, unsigned long timeout = 0, unsigned long transition_duration = 250) {
    Color current_color = getCurrentColor();
    start_color_ = current_color;
    override_color_ = color;

    transitioning_to_override_ = true;

    override_elapsed_time_ = 0;
    override_duration_ = transition_duration;
    override_timeout_ = timeout;
  }

  void override() {
    // We set it to 1 because it's the smallest number that will resume the transition
    if (overridden_) override_timeout_ = 1;
  }

  Color tick() {
    updateElapsedTime();

    if (flashing_) {
      float flash_progress = (float)flash_elapsed_time_ / single_flash_duration_;

      if (flash_progress >= 1.0f) {
        current_flash_++;
        flash_elapsed_time_ = 0;

        if (current_flash_ >= flash_count_) {
          flashing_ = false;

          return tick();
        }

        // Swap start and end colors for next fade
        Color temp = current_flash_start_;
        current_flash_start_ = current_flash_end_;
        current_flash_end_ = temp;
        flash_progress = 0;
      }

      return Color(
        current_flash_start_.r + ((current_flash_end_.r - current_flash_start_.r) * flash_progress),
        current_flash_start_.g + ((current_flash_end_.g - current_flash_start_.g) * flash_progress),
        current_flash_start_.b + ((current_flash_end_.b - current_flash_start_.b) * flash_progress));
    }

    if (transitioning_to_override_) {
      float progress = min(1.0f, (float)override_elapsed_time_ / override_duration_);

      if (progress >= 1.0f) {
        transitioning_from_override_ = false;
        overridden_ = true;
        override_elapsed_time_ = 0;
      }

      return Color(
        start_color_.r + (override_color_.r - start_color_.r) * progress,
        start_color_.g + (override_color_.g - start_color_.g) * progress,
        start_color_.b + (override_color_.b - start_color_.b) * progress);
    }

    if (overridden_) {
      if (override_timeout_ > 0 && override_elapsed_time_ >= override_timeout_) {
        overridden_ = false;
        transitioning_from_override_ = true;

        elapsed_time_ = 0;

        start_color_ = override_color_;
        end_color_ = getCurrentColor();
      } else {
        return override_color_;
      }
    }

    if (transitioning_from_override_) {
      float progress = min(1.0f, (float)elapsed_time_ / override_duration_);

      if (progress >= 1.0f) {
        transitioning_from_override_ = false;
      }

      return Color(
        start_color_.r + (end_color_.r - start_color_.r) * progress,
        start_color_.g + (end_color_.g - start_color_.g) * progress,
        start_color_.b + (end_color_.b - start_color_.b) * progress);
    }

    float progress = min(1.0f, (float)elapsed_time_ / duration_);

    Color current_color = getCurrentColor();

    if (progress >= 1.0f) {
      nextTransition();
    }

    return current_color;
  }

private:
  // ---------- Normal ---------- //

  Color start_color_;
  Color end_color_;
  Color last_color_;

  unsigned long elapsed_time_;
  unsigned long last_update_time_;
  unsigned long duration_;

  bool paused_;

  // ---------- Override ---------- //

  Color override_color_;

  bool overridden_;
  bool transitioning_from_override_;
  bool transitioning_to_override_;

  unsigned long override_elapsed_time_;
  unsigned long override_duration_;
  unsigned long override_timeout_;

  // ---------- Flash ---------- //

  Color flash_color_;
  Color current_flash_start_;
  Color current_flash_end_;

  bool flashing_;

  int flash_count_;
  int current_flash_;

  unsigned long flash_elapsed_time_;
  unsigned long single_flash_duration_;

  const Color RED{ 255, 0, 0 };
  const Color GREEN{ 0, 255, 0 };
  const Color BLUE{ 0, 0, 255 };

  void updateElapsedTime() {
    if (!paused_) {
      unsigned long current_time = millis();
      unsigned long delta_time = current_time - last_update_time_;

      last_update_time_ = current_time;

      // Update appropriate timer based on the current state
      if (flashing_) {
        flash_elapsed_time_ += delta_time;
      } else if (overridden_ || transitioning_to_override_) {
        override_elapsed_time_ += delta_time;
      } else {
        elapsed_time_ += delta_time;
      }
    }
  }

  Color getCurrentColor() {
    float progress = min(1.0f, (float)elapsed_time_ / duration_);

    return Color(
      start_color_.r + ((end_color_.r - start_color_.r) * progress),
      start_color_.g + ((end_color_.g - start_color_.g) * progress),
      start_color_.b + ((end_color_.b - start_color_.b) * progress));
  }

  void nextTransition() {
    start_color_ = end_color_;

    if (end_color_.b == 255) {         // End of color blue
      end_color_ = GREEN;              // Change color to Green
    } else if (end_color_.g == 255) {  // End of color green
      end_color_ = RED;                // Change color to Red
    } else if (end_color_.r == 255) {  // End of color Red
      end_color_ = BLUE;               // Change color to Blue
    }

    elapsed_time_ = 0;
  }
};
