#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Adafruit_EPD.h>

// Pin definitions from the original main.cpp
#define USER_LED 1
#define SAO1_GPIO1 2
#define EDP_CS2 3
#define SAO2_GPIO1 4
#define SAO2_GPIO2 5
#define SAO3_GPIO2 6
#define SAO3_GPIO1 7
#define EDP_BUSY2 8
#define TP8 9
#define EDP_DC 10
#define EDP_RST 11
#define EDP_BUSY1 12
#define EDP_CS1 13
#define BATT_SENSE 14
#define MENU_PLUS 15
#define USER_B 16
#define SAO_SDA 17
#define SAO_SCL 18
#define EDP_SCLK 21
#define NEOPIXEL_BAR 38
#define USER_A 39
#define MENU_MINUS 40
#define TP9 41
#define SAO1_GPIO2 42
#define EDP_MISO 45
#define EDP_MOSI 47
#define EDP_PWR_EN 48

#define NUMPIXELS 8

// NeoPixel setup
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_BAR, NEO_GRB + NEO_KHZ800);

// E-ink display pins
#define EINK_CS EDP_CS1
#define EINK_DC EDP_DC
#define EINK_RST EDP_RST
#define EINK_BUSY EDP_BUSY1
#define EINK_SCLK EDP_SCLK
#define EINK_MOSI EDP_MOSI
#define EINK_MISO EDP_MISO
#define EINK_PWR EDP_PWR_EN

// Display dimensions for Pervasive Displays E2969JS0B1 (2.13\" 250x122)
#define EINK_WIDTH 250
#define EINK_HEIGHT 122

Adafruit_IL0373 display(EINK_WIDTH, EINK_HEIGHT, EINK_DC, EINK_RST, EINK_CS, -1, EINK_BUSY, &SPI);

// Variables for e-ink display update interval
const unsigned long EINK_INTERVAL = 5000; // 5 seconds
unsigned long lastEInkUpdate = 0;


// Button pins
const uint8_t buttonPins[] = {USER_A, USER_B, MENU_PLUS, MENU_MINUS};
const char* buttonNames[] = {"USER_A", "USER_B", "MENU_PLUS", "MENU_MINUS"};
const size_t numButtons = sizeof(buttonPins) / sizeof(buttonPins[0]);



// Variables for button state tracking
bool buttonStates[numButtons];
bool lastButtonStates[numButtons];

// I2C scan interval (in ms)
const unsigned long I2C_SCAN_INTERVAL = 5000;
unsigned long lastI2CScan = 0;

// NeoPixel color cycle interval (in ms)
const unsigned long NEOPIXEL_INTERVAL = 100;
unsigned long lastNeoPixelUpdate = 0;
uint8_t neoPixelHue = 0;

// USER_LED blink interval (in ms)
const unsigned long LED_INTERVAL = 500;
unsigned long lastLedToggle = 0;
bool ledState = false;

void setup() {
  // Start serial communication
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect (needed for native USB)
  }
  Serial.println("\n\n=== HS2026 Badge Test Program ===");

  // Initialize USER_LED as output
  pinMode(USER_LED, OUTPUT);
  digitalWrite(USER_LED, ledState);

  // Initialize button pins as inputs with pull-up
  for (size_t i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    buttonStates[i] = digitalRead(buttonPins[i]);
    lastButtonStates[i] = buttonStates[i];
  }

  // Initialize NeoPixel bar
  pixels.begin();
  pixels.clear();
  pixels.show();

  // Initialize I2C bus (SAO_SDA, SAO_SCL)
  Wire.begin(SAO_SDA, SAO_SCL);
  Serial.println("I2C initialized on SAO_SDA/SAO_SCL");

  // Blink LED a few times to indicate setup is done
  for (int i = 0; i < 5; i++) {
    digitalWrite(USER_LED, !ledState);
    delay(100);
    digitalWrite(USER_LED, ledState);
    delay(100);
  }

  // Initialize e-ink display power
pinMode(EINK_PWR, OUTPUT);
digitalWrite(EINK_PWR, HIGH); // Power on the display

// Initialize the display
display.begin();
display.setRotation(0);
display.clearBuffer();

Serial.println("Setup complete. Entering loop...");
}

void loop() {
  unsigned long now = millis();

  // Blink USER_LED to indicate the program is running
  if (now - lastLedToggle >= LED_INTERVAL) {
    lastLedToggle = now;
    ledState = !ledState;
    digitalWrite(USER_LED, ledState);
  }

  // Update NeoPixel bar with a color cycle
  if (now - lastNeoPixelUpdate >= NEOPIXEL_INTERVAL) {
    lastNeoPixelUpdate = now;
    neoPixelHue = (neoPixelHue + 1) % 255;
    for (int i = 0; i < NUMPIXELS; i++) {
      // Optional: shift hue per pixel for a rainbow effect
      uint8_t hue = (neoPixelHue + (i * 255 / NUMPIXELS)) % 255;
      pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(hue)));
    }
    pixels.show();
  }

  // Check buttons
  for (size_t i = 0; i < numButtons; i++) {
    bool state = digitalRead(buttonPins[i]);
    if (state != lastButtonStates[i]) {
      buttonStates[i] = state;
      Serial.print("Button ");
      Serial.print(buttonNames[i]);
      Serial.print(": ");
      Serial.println(state ? "RELEASED" : "PRESSED");
      lastButtonStates[i] = state;
    }
  }

  // Scan I2C bus periodically
  if (now - lastI2CScan >= I2C_SCAN_INTERVAL) {
    lastI2CScan = now;
    Serial.println("Scanning I2C bus...");
    byte error, address;
    int nDevices = 0;
    for (address = 1; address < 127; address++ ) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0) {
        Serial.print("I2C device found at address 0x");
        if (address < 16) Serial.print("0");
        Serial.println(address, HEX);
        nDevices++;
      } else if (error == 4) {
        // Unknown error
        Serial.print("Unknown error at address 0x");
        if (address < 16) Serial.print("0");
        Serial.println(address, HEX);
      }
    }
    if (nDevices == 0) {
      Serial.println("No I2C devices found");
    } else {
      Serial.print("Found ");
      Serial.print(nDevices);
      Serial.println(" I2C device(s)");
    }
  }

    // Update e-ink display periodically
  if (now - lastEInkUpdate >= EINK_INTERVAL) {
    lastEInkUpdate = now;
    display.clearBuffer();
    display.fillRect(20, 20, EINK_WIDTH-40, EINK_HEIGHT-40, EPD_BLACK);
    display.display();
  }
// Small delay to avoid overwhelming the serial output
  delay(10);
}