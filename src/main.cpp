#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <vector>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <LittleFS.h>
#include <PNGdec.h>
#include "epd97.h"
#include "pins.h"

const char* WIFI_SSID = "Supercon";
const char* WIFI_PASS = "tink1930";

#ifndef APP_FEATURE_WIFI
#define APP_FEATURE_WIFI 1
#endif
#ifndef APP_FEATURE_BLE
#define APP_FEATURE_BLE 1
#endif
#ifndef APP_FEATURE_IMAGE
#define APP_FEATURE_IMAGE 1
#endif

struct HostPort {
    IPAddress ip;
    uint16_t port;
};

enum AppMode {
    APP_WIFI = 0,
    APP_BLE = 1,
    APP_IMAGE = 2,
    APP_INFO = 3
};

static AppMode g_selectedApp = APP_WIFI;
static const char* kImagePath = "/assets/supercon-x-splash.png";
static PNG g_activePng;
static uint16_t* g_activeLine = nullptr;
static bool g_pngReady = false;

static int pngDraw(PNGDRAW* pDraw) {
    if (!g_activeLine) {
        return 1;
    }

    g_activePng.getLineAsRGB565(pDraw, g_activeLine, PNG_RGB565_LITTLE_ENDIAN, 0);
    for (int x = 0; x < pDraw->iWidth; ++x) {
        uint16_t rgb = g_activeLine[x];
        uint8_t r = (rgb >> 11) & 0x1F;
        uint8_t g = (rgb >> 5) & 0x3F;
        uint8_t b = rgb & 0x1F;
        r = (r * 255u) / 31u;
        g = (g * 255u) / 63u;
        b = (b * 255u) / 31u;

        uint8_t lum = (r * 299u + g * 587u + b * 114u) / 1000u;
        int px = x;
        int py = pDraw->y;
        if (px >= 0 && px < EPD_W && py >= 0 && py < EPD_H && lum < 180) {
            epd_pixel(px, py, EPD_BLACK);
        }
    }
    return 1;
}

static bool renderPngAsset(const char* path) {
    if (!LittleFS.exists(path)) {
        Serial.printf("Missing image: %s\n", path);
        return false;
    }

    File imgFile = LittleFS.open(path, "r");
    if (!imgFile) {
        Serial.printf("Could not open image: %s\n", path);
        return false;
    }

    size_t fileSize = imgFile.size();
    uint8_t* imageData = (uint8_t*)malloc(fileSize);
    if (!imageData) {
        Serial.println("PNG alloc failed");
        imgFile.close();
        return false;
    }

    imgFile.read(imageData, fileSize);
    imgFile.close();

    epd_clear_screen();
    epd_text(10, 12, "Image", EPD_BLACK, 2);
    epd_text(10, 52, "loading...", EPD_BLACK, 2);
    refresh_display(EPD_SLOW);

    g_pngReady = false;
    int rc = g_activePng.openRAM(imageData, fileSize, pngDraw);
    if (rc != PNG_SUCCESS) {
        Serial.printf("PNG open failed: %d\n", rc);
        free(imageData);
        return false;
    }

    int w = g_activePng.getWidth();
    int h = g_activePng.getHeight();
    Serial.printf("PNG size: %d x %d\n", w, h);
    g_activeLine = (uint16_t*)malloc((size_t)w * sizeof(uint16_t));
    if (!g_activeLine) {
        Serial.println("PNG line buffer alloc failed");
        g_activePng.close();
        free(imageData);
        return false;
    }

    rc = g_activePng.decode(nullptr, 0);
    free(imageData);
    free(g_activeLine);
    g_activeLine = nullptr;
    g_activePng.close();

    if (rc != PNG_SUCCESS) {
        Serial.printf("PNG decode failed: %d\n", rc);
        return false;
    }

    g_pngReady = true;
    return true;
}

static void drawStatus(const char* title, const char* line1 = nullptr, const char* line2 = nullptr) {
    epd_text(10, 12, title, EPD_BLACK, 2);
    if (line1) epd_text(10, 52, line1, EPD_BLACK, 2);
    if (line2 && line2[0] != '\0') epd_text(10, 82, line2, EPD_BLACK, 2);
}

static void showText(const char* title, const char* line1 = nullptr, const char* line2 = nullptr) {
    epd_clear_screen();
    drawStatus(title, line1, line2);
    refresh_display(EPD_SLOW);
}

static void showStatus(const char* title, const char* line1 = nullptr, const char* line2 = nullptr) {
    drawStatus(title, line1, line2);
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

static bool appEnabled(AppMode app) {
    switch (app) {
        case APP_WIFI:
            return APP_FEATURE_WIFI != 0;
        case APP_BLE:
            return APP_FEATURE_BLE != 0;
        case APP_IMAGE:
            return APP_FEATURE_IMAGE != 0;
        case APP_INFO:
            return true;
    }
    return false;
}

static void normalizeSelection() {
    if (!appEnabled(g_selectedApp)) {
        if (APP_FEATURE_WIFI) {
            g_selectedApp = APP_WIFI;
        } else if (APP_FEATURE_BLE) {
            g_selectedApp = APP_BLE;
        } else if (APP_FEATURE_IMAGE) {
            g_selectedApp = APP_IMAGE;
        } else {
            g_selectedApp = APP_INFO;
        }
    }
}

static void showModeMenu() {
    normalizeSelection();
    epd_clear_screen();
    epd_text(10, 12, "Apps", EPD_BLACK, 2);

    int y = 42;
#if APP_FEATURE_WIFI
    char wifiLine[32];
    snprintf(wifiLine, sizeof(wifiLine), "%s WiFi Scan", (g_selectedApp == APP_WIFI) ? ">" : " ");
    epd_text(10, y, wifiLine, EPD_BLACK, 2);
    y += 22;
#endif

#if APP_FEATURE_BLE
    char bleLine[32];
    snprintf(bleLine, sizeof(bleLine), "%s BLE Scan", (g_selectedApp == APP_BLE) ? ">" : " ");
    epd_text(10, y, bleLine, EPD_BLACK, 2);
    y += 22;
#endif

#if APP_FEATURE_IMAGE
    char imageLine[32];
    snprintf(imageLine, sizeof(imageLine), "%s Image", (g_selectedApp == APP_IMAGE) ? ">" : " ");
    epd_text(10, y, imageLine, EPD_BLACK, 2);
    y += 22;
#endif

    char infoLine[32];
    snprintf(infoLine, sizeof(infoLine), "%s Info", (g_selectedApp == APP_INFO) ? ">" : " ");
    epd_text(10, y, infoLine, EPD_BLACK, 2);

    refresh_display(EPD_SLOW);
}

static void showInfoScreen() {
    epd_clear_screen();
    epd_text(10, 12, "Badge", EPD_BLACK, 2);
    epd_text(10, 42, "Menu+/- move", EPD_BLACK, 2);
    epd_text(10, 68, "A select", EPD_BLACK, 2);
    epd_text(10, 94, "B back", EPD_BLACK, 2);
    refresh_display(EPD_SLOW);
}

static void showImageApp() {
    Serial.printf("Loading image from %s\n", kImagePath);
    if (!renderPngAsset(kImagePath)) {
        showText("Image", "missing", "PNG asset");
        return;
    }

    Serial.println("Image render complete");
    refresh_display(EPD_SLOW);
}

static void scanBle() {
    epd_clear_screen();
    showStatus("BLE scan", "scanning...", "");

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

    epd_clear_screen();
    showStatus("LAN scan", "scanning...", "");

    IPAddress localIp = WiFi.localIP();
    IPAddress mask = WiFi.subnetMask();
    byte base[4];
    for (int i = 0; i < 4; ++i) {
        base[i] = localIp[i] & mask[i];
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
        if (ip == localIp) {
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

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
    } else {
        Serial.println("LittleFS ready");
    }

    pinMode(MENU_PLUS, INPUT_PULLUP);
    pinMode(MENU_MINUS, INPUT_PULLUP);
    pinMode(USER_A, INPUT_PULLUP);
    pinMode(USER_B, INPUT_PULLUP);

    if (!epd_begin()) {
        Serial.println("EPD init failed");
        return;
    }

    showModeMenu();
}

void loop() {
    if (buttonPressed(MENU_PLUS)) {
        while (buttonPressed(MENU_PLUS)) {
            delay(20);
        }

        AppMode current = g_selectedApp;
        do {
            if (current > APP_WIFI) {
                current = static_cast<AppMode>(current - 1);
            } else {
                current = APP_INFO;
            }
        } while (!appEnabled(current) && current != g_selectedApp);
        g_selectedApp = current;
        showModeMenu();
        return;
    }

    if (buttonPressed(MENU_MINUS)) {
        while (buttonPressed(MENU_MINUS)) {
            delay(20);
        }

        AppMode current = g_selectedApp;
        do {
            if (current < APP_INFO) {
                current = static_cast<AppMode>(current + 1);
            } else {
                current = APP_WIFI;
            }
        } while (!appEnabled(current) && current != g_selectedApp);
        g_selectedApp = current;
        showModeMenu();
        return;
    }

    if (buttonPressed(USER_A)) {
        while (buttonPressed(USER_A)) {
            delay(20);
        }

        switch (g_selectedApp) {
#if APP_FEATURE_WIFI
            case APP_WIFI: {
                WiFi.mode(WIFI_STA);
                WiFi.begin(WIFI_SSID, WIFI_PASS);
                epd_clear_screen();
                showStatus("Wifi", "connecting...", "");

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
#endif

#if APP_FEATURE_BLE
            case APP_BLE:
                scanBle();
                delay(2000);
                showModeMenu();
                return;
#endif

#if APP_FEATURE_IMAGE
            case APP_IMAGE:
                showImageApp();
                delay(2000);
                showModeMenu();
                return;
#endif

            case APP_INFO:
                showInfoScreen();
                delay(2000);
                showModeMenu();
                return;
        }
    }

    if (buttonPressed(USER_B)) {
        while (buttonPressed(USER_B)) {
            delay(20);
        }
        showModeMenu();
        return;
    }

    delay(50);
}