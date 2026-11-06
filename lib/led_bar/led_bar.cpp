#include "led_bar.h"

LedBar::LedBar() : strip_(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800) {}

void LedBar::begin() {
    strip_.begin();
    strip_.setBrightness(50);
    strip_.show();
}

void LedBar::fill(RGB c) {
    for (int i = 0; i < NUM_LEDS; i++) {
        strip_.setPixelColor(i, strip_.Color(c.r, c.g, c.b));
    }
}

void LedBar::setPixel(uint8_t idx, RGB c) {
    if (idx < NUM_LEDS) {
        strip_.setPixelColor(idx, strip_.Color(c.r, c.g, c.b));
    }
}

void LedBar::show() { strip_.show(); }

void LedBar::clear() {
    fill({0, 0, 0});
    show();
}

void LedBar::menuDisplay(uint8_t selected, uint8_t total) {
    // Simple scrolling indicator
    clear();
    uint8_t pos = selected % NUM_LEDS;
    setPixel(pos, {3, 2, 1});  // white
    show();
}