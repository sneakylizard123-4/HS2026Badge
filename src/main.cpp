#include <Arduino.h>
#include <SPI.h>
#include <LittleFS.h>
#include <PNGdec.h>
#include "epd97.h"
#include "pins.h"

namespace {

void *pngOpen(const char *filename, int32_t *size)
{
    static File file;
    file = LittleFS.open(filename, FILE_READ);
    if (!file) {
        *size = 0;
        return NULL;
    }
    *size = file.size();
    return &file;
}

void pngClose(void *handle)
{
    File *file = static_cast<File *>(handle);
    if (file) {
        file->close();
    }
}

int32_t pngRead(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
    File *file = static_cast<File *>(handle->fHandle);
    if (!file) {
        return 0;
    }
    return file->read(buffer, length);
}

int32_t pngSeek(PNGFILE *handle, int32_t position)
{
    File *file = static_cast<File *>(handle->fHandle);
    if (!file) {
        return 0;
    }
    return file->seek(position);
}

int pngDraw(PNGDRAW *pDraw)
{
    PNG *png = reinterpret_cast<PNG *>(pDraw->pUser);
    if (!png || pDraw->iWidth > EPD_W) {
        return 0;
    }

    static uint16_t line[EPD_W];
    png->getLineAsRGB565(pDraw, line, PNG_RGB565_BIG_ENDIAN, 0);

    const int x0 = (EPD_W - png->getWidth()) / 2;
    const int y0 = (EPD_H - png->getHeight()) / 2;

    for (int x = 0; x < pDraw->iWidth; ++x) {
        const uint16_t pixel = line[x];
        const uint8_t r = (pixel >> 11) & 0x1F;
        const uint8_t g = (pixel >> 5) & 0x3F;
        const uint8_t b = pixel & 0x1F;

        const uint8_t rr = (r * 255U) / 31U;
        const uint8_t gg = (g * 255U) / 63U;
        const uint8_t bb = (b * 255U) / 31U;
        const uint8_t brightness = (rr * 299U + gg * 587U + bb * 114U) / 1000U;

        const int drawX = x0 + x;
        const int drawY = y0 + pDraw->y;
        if (drawX >= 0 && drawX < EPD_W && drawY >= 0 && drawY < EPD_H) {
            epd_pixel(drawX, drawY, brightness < 128 ? EPD_BLACK : EPD_WHITE);
        }
    }

    return 1;
}

bool displayPng(const char *path)
{
    PNG png;
    const int openRc = png.open(path, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    if (openRc != PNG_SUCCESS) {
        Serial.printf("PNG open failed for %s: %d\n", path, openRc);
        return false;
    }

    const int w = png.getWidth();
    const int h = png.getHeight();
    if (w <= 0 || h <= 0 || w > EPD_W || h > EPD_H) {
        Serial.printf("PNG size out of range: %dx%d\n", w, h);
        png.close();
        return false;
    }

    const int decodeRc = png.decode(&png, 0);
    png.close();
    if (decodeRc != PNG_SUCCESS) {
        Serial.printf("PNG decode failed for %s: %d\n", path, decodeRc);
        return false;
    }

    return true;
}

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("HS2026 badge boot");

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
        return;
    }

    if (!epd_begin()) {
        Serial.println("epd_begin failed");
        return;
    }

    const char *helloPng = "/assets/hello_i_am_blank.png";
    epd_fill(EPD_WHITE);

    if (!LittleFS.exists(helloPng)) {
        Serial.printf("Missing PNG: %s\n", helloPng);
        epd_fill(EPD_WHITE);
        epd_text(40, 40, "Missing", EPD_BLACK, 6);
        epd_text(40, 120, "PNG", EPD_BLACK, 6);
        refresh_display(EPD_SLOW);
        return;
    }

    if (!displayPng(helloPng)) {
        epd_fill(EPD_WHITE);
        epd_text(40, 40, "Decode", EPD_BLACK, 6);
        epd_text(40, 120, "FAILED", EPD_BLACK, 6);
        refresh_display(EPD_SLOW);
        return;
    }

    refresh_display(EPD_SLOW);
    Serial.printf("Displayed PNG: %s\n", helloPng);
}

void loop()
{
    delay(1000);
}
