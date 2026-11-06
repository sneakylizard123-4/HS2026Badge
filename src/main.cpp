#include <Arduino.h>
#include "epd_driver.h"
#include "button_manager.h"
#include "led_bar.h"
#include "app_switcher.h"

// Global objects
EpdDriver epd;
ButtonManager buttons;
LedBar leds;
AppSwitcher switcher;

void setup() {
    Serial.begin(115200);
    delay(500);

    // Initialise EPD
    epd.begin();

    // Initialise LEDs
    leds.begin();
    leds.fill({0, 1, 1});  // Splash colour
    leds.show();

    // Draw splash screen
    epd.clear();
    epd.drawPng("/assets/supercon-x-splash.png");
    epd.update();

    // Initialise buttons and app switcher
    buttons.begin();
    switcher.begin(&epd, &buttons, &leds);

    Serial.println("Badge ready");
}

void loop() {
    // Update button states
    buttons.update();

    // Run the active app
    switcher.update();

    // Small delay to avoid busy‑looping
    delay(5);
}