#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <vector>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include "epd97.h"
#include "pins.h"

const char* WIFI_SSID = "Supercon";
const char* WIFI_PASS = "tink1930";

struct HostPort {
    IPAddress ip;
    uint16_t port;
};

static void showText(const char* title, const char* line1 = nullptr, const char* line2 = nullptr) {
    epd_clear_screen();
    epd_text(10, 12, title, EPD_BLACK, 2);
    if (line1) epd_text(10, 52, line1, EPD_BLACK, 2);
    if (line2 && line2[0] != '\0') epd_text(10, 82, line2, EPD_BLACK, 2);
    refresh_display(EPD_SLOW);
}

static void showScanResults(const std::vector<HostPort>& results) {
    Serial.println("Scan results:");
    for (size_t i = 0; i < results.size() && i < 8; ++i) {
        String ipText = results[i].ip.toString();
        Serial.printf("  %s:%u\n", ipText.c_str(), results[i].port);
    }

    epd_clear_screen();
    epd_text(10, 10, "LAN scan", EPD_BLACK, 2);

    int y = 38;
    for (size_t i = 0; i < results.size() && i < 8; ++i) {
        String ipText = results[i].ip.toString();
        char line[64];
        snprintf(line, sizeof(line), "%s:%u",
                 ipText.c_str(),
                 results[i].port);
        epd_text(10, y, line, EPD_BLACK, 2);
        y += 22;
    }

    refresh_display(EPD_SLOW);
}

static bool buttonPressed(int pin) {
    return digitalRead(pin) == LOW;
}

static void showModeMenu() {
    epd_clear_screen();
    epd_text(10, 12, "Badge", EPD_BLACK, 2);
    epd_text(10, 52, "A: WiFi", EPD_BLACK, 2);
    epd_text(10, 82, "B: BLE", EPD_BLACK, 2);
    refresh_display(EPD_SLOW);
}

static void scanBle() {
    epd_clear_screen();
    epd_text(10, 12, "BLE scan", EPD_BLACK, 2);
    epd_text(10, 52, "scanning...", EPD_BLACK, 2);
    refresh_display(EPD_SLOW);

    BLEDevice::init("");
    BLEScan* scan = BLEDevice::getScan();
    if (!scan) {
        Serial.println("BLE scan unavailable");
        showText("BLE", "not available", "");
        return;
    }

    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);

    Serial.println("Starting BLE scan...");
    BLEScanResults* results = scan->start(3, false);
    int count = results->getCount();
    Serial.printf("BLE scan found %d devices\n", count);

    epd_clear_screen();
    epd_text(10, 10, "BLE scan", EPD_BLACK, 2);

    if (count == 0) {
        epd_text(10, 40, "No devices", EPD_BLACK, 2);
        refresh_display(EPD_SLOW);
        Serial.println("No BLE devices found");
        return;
    }

    int y = 40;
    for (int i = 0; i < count && i < 8; ++i) {
        BLEAdvertisedDevice device = results->getDevice(i);
        String name = device.getName().c_str();
        if (name.length() == 0) {
            name = "unnamed";
        }

        Serial.printf("  %s (%d dBm)\n", name.c_str(), device.getRSSI());
        char line[64];
        snprintf(line, sizeof(line), "%s %d", name.c_str(), device.getRSSI());
        epd_text(10, y, line, EPD_BLACK, 2);
        y += 22;
    }

    refresh_display(EPD_SLOW);
}

static void scanLan() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping LAN scan");
        return;
    }

    showText("LAN scan", "scanning...", "");

    IPAddress local = WiFi.localIP();
    IPAddress mask = WiFi.subnetMask();
    byte base[4];
    for (int i = 0; i < 4; ++i) {
        base[i] = local[i] & mask[i];
    }

    std::vector<HostPort> hits;
    hits.reserve(8);
    const uint16_t ports[] = {22, 80, 443, 8080};
    const int scanMaxHost = 48;

    Serial.printf("Scanning %d.%d.%d.* using subnet mask %d.%d.%d.%d\n",
                  base[0], base[1], base[2], mask[0], mask[1], mask[2], mask[3]);
    uint32_t startMs = millis();

    for (int host = 1; host < scanMaxHost && hits.size() < 8; ++host) {
        IPAddress ip(base[0], base[1], base[2], host);
        if (ip == local) {
            continue;
        }

        Serial.printf("Checking %s\n", ip.toString().c_str());
        for (size_t p = 0; p < sizeof(ports) / sizeof(ports[0]); ++p) {
            WiFiClient client;
            client.setTimeout(30);
            if (client.connect(ip, ports[p])) {
                hits.push_back({ip, ports[p]});
                Serial.printf("Found open port on %s:%u\n", ip.toString().c_str(), ports[p]);
                client.stop();
                break;
            }
        }
    }

    Serial.printf("Scan finished in %lu ms\n", millis() - startMs);

    if (hits.empty()) {
        Serial.printf("No hosts with open ports found on %s/%s\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.subnetMask().toString().c_str());
        Serial.println("This is often caused by Wi-Fi client isolation or by no reachable hosts/services on the LAN.");
        showText("LAN scan", "No hosts", "with ports");
        return;
    }

    Serial.printf("Displaying %zu scan results\n", hits.size());
    showScanResults(hits);
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    pinMode(USER_A, INPUT_PULLUP);
    pinMode(USER_B, INPUT_PULLUP);

    if (!epd_begin()) {
        Serial.println("EPD init failed");
        return;
    }

    showModeMenu();
}

void loop() {
    if (buttonPressed(USER_A)) {
        while (buttonPressed(USER_A)) {
            delay(20);
        }

        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        showText("Wifi", "connecting...", "");

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
            delay(250);
            Serial.print(".");
        }

        if (WiFi.status() != WL_CONNECTED) {
            showText("Wifi", "not connected", "");
            delay(1500);
            showModeMenu();
            return;
        }

        Serial.println("\nWiFi connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

        String ipText = WiFi.localIP().toString();
        Serial.printf("Showing IP on EPD: %s\n", ipText.c_str());
        showText("Wifi", "connected", ipText.c_str());
        delay(1000);
        scanLan();
        epd_set_keep_awake(true);
        delay(2000);
        showModeMenu();
        return;
    }

    if (buttonPressed(USER_B)) {
        while (buttonPressed(USER_B)) {
            delay(20);
        }
        scanBle();
        delay(2000);
        showModeMenu();
        return;
    }

    delay(50);
}