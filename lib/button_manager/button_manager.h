#pragma once
#include <Arduino.h>

enum ButtonState { UNPRESSED, PRESSED_UNREAD, PRESSED_READ };

class Button {
public:
    Button(uint8_t pin, const char* name);
    void begin();
    void update();
    bool pressed();          // true once per press
    uint32_t pressedMs() const;

private:
    uint8_t pin_;
    const char* name_;
    bool value_ = false;
    ButtonState state_ = UNPRESSED;
    uint32_t pressOnset_ = 0;
};

class ButtonManager {
public:
    ButtonManager();
    void begin();
    void update();
    Button& menuMinus() { return menuMinus_; }
    Button& menuPlus()  { return menuPlus_; }
    Button& userA()     { return userA_; }
    Button& userB()     { return userB_; }

private:
    Button menuMinus_{40, "Menu -"};
    Button menuPlus_{15, "Menu +"};
    Button userA_{39, "User A"};
    Button userB_{16, "User B"};
};