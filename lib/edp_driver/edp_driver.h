#pragma once
#include <Arduino.h>
#include "EPD_Driver_GU_large.h"   // Pervasive Displays library

class EpdDriver {
public:
    EpdDriver();
    void begin();
    void clear();
    void drawPng(const char* path);
    void update();

private:
    // Pin definitions from the original firmware[reference:1]
    static const uint8_t CS1 = 13;
    static const uint8_t CS2 = 3;
    static const uint8_t BUSY1 = 12;
    static const uint8_t BUSY2 = 8;
    static const uint8_t DC = 10;
    static const uint8_t RST = 11;
    static const uint8_t SCL = 21;
    static const uint8_t SDA = 47;
    static const uint8_t PWR_EN = 48;
};