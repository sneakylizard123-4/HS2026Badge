#pragma once
#include <Adafruit_NeoPixel.h>

struct RGB { uint8_t r, g, b; };

class LedBar {
public:
    LedBar();
    void begin();
    void fill(RGB colour);
    void setPixel(uint8_t idx, RGB colour);
    void show();
    void clear();
    void menuDisplay(uint8_t selected, uint8_t total);

private:
    static const uint8_t NUM_LEDS = 8;
    static const uint8_t PIN = 38;
    Adafruit_NeoPixel strip_;
};