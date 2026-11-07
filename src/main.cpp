#include <Arduino.h>
#include <SPI.h>
#include <LittleFS.h>
#include <PNGdec.h>
#include "epd97.h"
#include "pins.h"

namespace {

constexpr uint8_t kButtonPins[] = {USER_A, USER_B, MENU_PLUS, MENU_MINUS};
constexpr size_t kButtonCount = sizeof(kButtonPins) / sizeof(kButtonPins[0]);

bool lastButtonStates[kButtonCount] = {false, false, false, false};
int currentIndex = 0;

void *pngOpen(const char *filename, int32_t *size)
{
    static File myfile;
    myfile = LittleFS.open(filename, FILE_READ);
    if (!myfile) {
        *size = 0;
        return NULL;
    }
    *size = myfile.size();
    return &myfile;
}

void pngClose(void *handle)
{
    File *pFile = static_cast<File *>(handle);
    if (pFile) {
        pFile->close();
    }
}

int32_t pngRead(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
    File *pFile = static_cast<File *>(handle->fHandle);
    if (!pFile) {
        return 0;
    }
    return pFile->read(buffer, length);
}

int32_t pngSeek(PNGFILE *handle, int32_t position)
{
    File *pFile = static_cast<File *>(handle->fHandle);
    if (!pFile) {
        return 0;
    }
    return pFile->seek(position);
}

int PNGDraw(PNGDRAW *pDraw)
{
    PNG *png = reinterpret_cast<PNG *>(pDraw->pUser);
    if (!png) {
        return 0;
    }

    static uint16_t line[EPD_W];
    if (pDraw->iWidth > EPD_W) {
        return 0;
    }

    png->getLineAsRGB565(pDraw, line, PNG_RGB565_BIG_ENDIAN, 0);

    const int x0 = (EPD_W - png->getWidth()) / 2;
    const int y0 = (EPD_H - png->getHeight()) / 2;

    for (int x = 0; x < pDraw->iWidth; ++x) {
        uint16_t pixel = line[x];
        uint8_t r = (pixel >> 11) & 0x1F;
        uint8_t g = (pixel >> 5) & 0x3F;
        uint8_t b = pixel & 0x1F;

        uint8_t rr = (r * 255U) / 31U;
        uint8_t gg = (g * 255U) / 63U;
        uint8_t bb = (b * 255U) / 31U;
        uint8_t brightness = (rr * 299U + gg * 587U + bb * 114U) / 1000U;

        const int drawX = x0 + x;
        const int drawY = y0 + pDraw->y;

        if (drawX >= 0 && drawX < EPD_W && drawY >= 0 && drawY < EPD_H) {
            epd_pixel(drawX, drawY, brightness < 160 ? EPD_BLACK : EPD_WHITE);
        }
    }

    return 1;
}

bool displayPng(const char *path)
{
    PNG png;
    const int rc = png.open(path, pngOpen, pngClose, pngRead, pngSeek, PNGDraw);
    if (rc != PNG_SUCCESS) {
        Serial.printf("PNG open failed for %s: %d\n", path, rc);
        return false;
    }

    const int w = png.getWidth();
    const int h = png.getHeight();
    if (w <= 0 || h <= 0 || w > EPD_W || h > EPD_H) {
        Serial.printf("PNG size out of range: %dx%d\n", w, h);
        png.close();
        return false;
    }

    if (png.decode(&png, 0) != PNG_SUCCESS) {
        Serial.printf("PNG decode failed for %s\n", path);
        png.close();
        return false;
    }

    png.close();
    return true;
}

bool buttonPressed(uint8_t pin)
{
    return digitalRead(pin) == LOW;
}

int countPngFiles()
{
    int count = 0;

    File dir = LittleFS.open("/assets");
    if (dir && dir.isDirectory()) {
        while (File entry = dir.openNextFile()) {
            const String name = entry.name();
            if (!entry.isDirectory() && name.endsWith(".png")) {
                ++count;
            }
            entry.close();
        }
        dir.close();
    }

    if (count > 0) {
        return count;
    }

    File rootDir = LittleFS.open("/");
    if (!rootDir || !rootDir.isDirectory()) {
        return 0;
    }

    while (File entry = rootDir.openNextFile()) {
        const String name = entry.name();
        if (!entry.isDirectory() && name.endsWith(".png")) {
            ++count;
        }
        entry.close();
    }
    rootDir.close();
    return count;
}

bool showImageIndex(int index)
{
    const char *paths[] = {
        "/assets/%03d.png",
        "/%03d.png"
    };

    char path[64];
    bool ok = false;
    for (size_t i = 0; i < 2; ++i) {
        snprintf(path, sizeof(path), paths[i], index);
        epd_clear_screen();
        epd_clear();
        if (displayPng(path)) {
            ok = true;
            Serial.printf("Displayed %s\n", path);
            refresh_display(EPD_SLOW);
            break;
        }
    }

    if (!ok) {
        epd_fill(EPD_WHITE);
        epd_text(40, 40, "Missing", EPD_BLACK, 6);
        epd_text(40, 120, "PNG", EPD_BLACK, 6);
        refresh_display(EPD_SLOW);
    }
    return ok;
}

void handleNavigation(int delta)
{
    const int total = countPngFiles();
    if (total <= 0) {
        currentIndex = 0;
        showImageIndex(0);
        return;
    }

    currentIndex = (currentIndex + delta + total) % total;
    showImageIndex(currentIndex);
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("HS2026 badge boot");

    for (size_t i = 0; i < kButtonCount; ++i) {
        pinMode(kButtonPins[i], INPUT_PULLUP);
    }

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
        return;
    }

    if (!epd_begin()) {
        Serial.println("epd_begin failed");
        return;
    }

    int total = countPngFiles();
    if (total <= 0) {
        Serial.println("No PNG files found in /assets");
        epd_fill(EPD_WHITE);
        epd_text(40, 40, "No PNG", EPD_BLACK, 6);
        refresh_display(EPD_SLOW);
        return;
    }

    currentIndex = 0;
    showImageIndex(currentIndex);
}

void loop()
{
    bool wasPressed[kButtonCount];
    for (size_t i = 0; i < kButtonCount; ++i) {
        wasPressed[i] = lastButtonStates[i];
        lastButtonStates[i] = buttonPressed(kButtonPins[i]);
    }

    if (buttonPressed(USER_A) && !wasPressed[0]) {
        handleNavigation(-1);
    }
    if (buttonPressed(USER_B) && !wasPressed[1]) {
        handleNavigation(1);
    }
    if (buttonPressed(MENU_PLUS) && !wasPressed[2]) {
        handleNavigation(-1);
    }
    if (buttonPressed(MENU_MINUS) && !wasPressed[3]) {
        handleNavigation(1);
    }

    delay(50);
}

