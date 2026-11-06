#include <Arduino.h>
#include "pins.h"

// Minimal test: blink USER_LED and print to serial

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; } // wait for serial monitor
  Serial.println("\n=== HS2026 Badge Minimal Test ===");
  pinMode(USER_LED, OUTPUT);
  digitalWrite(USER_LED, LOW);
}

void loop() {
  static unsigned long lastToggle = 0;
  if (millis() - lastToggle >= 500) {
    lastToggle = millis();
    bool state = digitalRead(USER_LED);
    digitalWrite(USER_LED, !state);
    Serial.printf("LED toggled to %s\n", state ? "OFF" : "ON");
  }
}