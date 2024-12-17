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
      }
    }

    last_state_ = current_state;

    // Handle double-click timeout
    if (ready_for_double_click_ && ((current_time - last_press_time_) > double_click_timeout_)) {
      ready_for_double_click_ = false;
      click_count_ = 0;
    }
  }

  bool clicked() {
    if (single_click_detected_) {
      single_click_detected_ = false;

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
      single_click_detected_ = true;
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

  bool single_click_detected_ = false;
  bool double_click_detected_ = false;
};