#include <Arduino.h>
#include <SPI.h>
#include "epd97.h"
#include "pins.h"

void setup() {
    Serial.begin(115200);
    delay(1500);
    if (!epd_begin()) return;

    Serial.println("\nBOOT OK");
    Serial.printf("Free heap: %u\n", ESP.getFreeHeap());

    Serial.println("Initializing SPI...");

    SPI.begin(EDP_SCLK, -1, EDP_MOSI, -1);

    Serial.println("SPI initialized successfully!");
    epd_clear_screen();
    epd_set_keep_awake(true);
}

void loop() {
    delay(1000);
}