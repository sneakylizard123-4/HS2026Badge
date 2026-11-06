#include "app_switcher.h"

AppSwitcher::AppSwitcher() {}

void AppSwitcher::begin(EpdDriver* epd, ButtonManager* btn, LedBar* leds) {
    epd_ = epd;
    btn_ = btn;
    leds_ = leds;
    leds_->menuDisplay(selectionIdx_, NUM_APPS);
}

void AppSwitcher::update() {
    if (selecting_) {
        if (btn_->menuPlus().pressed()) {
            selectionIdx_ = (selectionIdx_ + 1) % NUM_APPS;
            leds_->menuDisplay(selectionIdx_, NUM_APPS);
        }
        if (btn_->menuMinus().pressed()) {
            selectionIdx_ = (selectionIdx_ - 1 + NUM_APPS) % NUM_APPS;
            leds_->menuDisplay(selectionIdx_, NUM_APPS);
        }
        if (btn_->userA().pressed() || btn_->userB().pressed()) {
            activeIdx_ = selectionIdx_;
            selecting_ = false;
            leds_->clear();
            // Launch the selected app (implement per‑app logic)
        }
    } else {
        // If Menu button pressed while an app is running, go back to menu
        if (btn_->menuMinus().pressed() || btn_->menuPlus().pressed()) {
            selectionIdx_ = activeIdx_;
            selecting_ = true;
            leds_->menuDisplay(selectionIdx_, NUM_APPS);
        }
        // Run the active app's update function
    }
}