#include "epd_driver.h"

EpdDriver::EpdDriver() {}

void EpdDriver::begin() {
    // Initialise SPI and control pins
    pinMode(CS1, OUTPUT);
    pinMode(CS2, OUTPUT);
    pinMode(DC, OUTPUT);
    pinMode(RST, OUTPUT);
    pinMode(PWR_EN, OUTPUT);
    digitalWrite(PWR_EN, HIGH);
    delay(10);

    // Pervasive Displays library initialisation
    EPD_Driver_GU_large::init(SCL, SDA, CS1, CS2, BUSY1, BUSY2, DC, RST);
    EPD_Driver_GU_large::setPanelSize(960, 672);
}

void EpdDriver::clear() {
    EPD_Driver_GU_large::clear(EPD_WHITE);
}

void EpdDriver::drawPng(const char* path) {
    // Placeholder: use LVGL or a PNG decoder to render the image
    // The original used LVGL's image widget[reference:2].
    // You can integrate LVGL here or call a custom draw routine.
}

void EpdDriver::update() {
    EPD_Driver_GU_large::update();
}