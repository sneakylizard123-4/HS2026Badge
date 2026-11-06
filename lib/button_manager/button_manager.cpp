#include "button_manager.h"

Button::Button(uint8_t pin, const char* name) : pin_(pin), name_(name) {}

void Button::begin() {
    pinMode(pin_, INPUT_PULLUP);
}

void Button::update() {
    bool current = !digitalRead(pin_);  // active‑low
    if (current && state_ == UNPRESSED) {
        state_ = PRESSED_UNREAD;
        pressOnset_ = millis();
    }
    if (!current) {
        state_ = UNPRESSED;
        pressOnset_ = 0;
    }
    value_ = current;
}

bool Button::pressed() {
    if (state_ == PRESSED_UNREAD) {
        state_ = PRESSED_READ;
        return true;
    }
    return false;
}

uint32_t Button::pressedMs() const {
    if (state_ != UNPRESSED && pressOnset_ != 0) {
        return millis() - pressOnset_;
    }
    return 0;
}

ButtonManager::ButtonManager() {}
void ButtonManager::begin() {
    menuMinus_.begin();
    menuPlus_.begin();
    userA_.begin();
    userB_.begin();
}
void ButtonManager::update() {
    menuMinus_.update();
    menuPlus_.update();
    userA_.update();
    userB_.update();
}