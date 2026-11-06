#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <vector>
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
    if (line2) epd_text(10, 82, line2, EPD_BLACK, 2);
    refresh_display(EPD_SLOW);
}

static void showScanResults(const std::vector<HostPort>& results) {
    epd_clear_screen();
    epd_text(10, 10, "LAN scan", EPD_BLACK, 2);

    int y = 38;
    for (size_t i = 0; i < results.size() && i < 10; ++i) {
        char line[64];
        snprintf(line, sizeof(line), "%s:%u",
                 results[i].ip.toString().c_str(),
                 results[i].port);
        epd_text(10, y, line, EPD_BLACK, 2);
        y += 20;
    }

    refresh_display(EPD_SLOW);
}

static void scanLan() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    IPAddress local = WiFi.localIP();
    IPAddress mask = WiFi.subnetMask();
    byte base[4];
    for (int i = 0; i < 4; ++i) {
        base[i] = local[i] & mask[i];
    }

    std::vector<HostPort> hits;
    const int ports[] = {22, 80, 443, 8080, 3306, 5900};

    for (int host = 2; host < 50 && hits.size() < 10; ++host) {
        IPAddress ip(base[0], base[1], base[2], host);
        for (size_t p = 0; p < sizeof(ports) / sizeof(ports[0]); ++p) {
            WiFiClient client;
            client.setTimeout(30);
            if (client.connect(ip, ports[p])) {
                hits.push_back({ip, (uint16_t)ports[p]});
                client.stop();
                break;
            }
        }
    }

    if (hits.empty()) {
        showText("LAN scan", "No hosts", "found");
        return;
    }

    showScanResults(hits);
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    if (!epd_begin()) {
        Serial.println("EPD init failed");
        return;
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
        return;
    }

    Serial.println("\nWiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    showText("Wifi", "connected", WiFi.localIP().toString().c_str());
    scanLan();
    epd_set_keep_awake(true);
}

void loop() {
    delay(60000);
    scanLan();
}