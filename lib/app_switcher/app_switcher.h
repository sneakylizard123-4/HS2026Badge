#pragma once
#include "epd_driver.h"
#include "button_manager.h"
#include "led_bar.h"

class AppSwitcher {
public:
    AppSwitcher();
    void begin(EpdDriver* epd, ButtonManager* btn, LedBar* leds);
    void update();

private:
    EpdDriver* epd_;
    ButtonManager* btn_;
    LedBar* leds_;
    bool selecting_ = true;
    uint8_t activeIdx_ = 0;
    uint8_t selectionIdx_ = 0;
    static const uint8_t NUM_APPS = 3;  // adjust as needed
};