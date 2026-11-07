#include <Arduino.h>
#include <SPI.h>
#include <LittleFS.h>
#include <PNGdec.h>
#include "epd97.h"
#include "pins.h"

static void *pngOpen(const char *filename, int32_t *size)
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

static void pngClose(void *handle)
{
    File *pFile = static_cast<File *>(handle);
    if (pFile) {
        pFile->close();
    }
}

static int32_t pngRead(PNGFILE *handle, uint8_t *buffer, int32_t length)
{
    File *pFile = static_cast<File *>(handle->fHandle);
    if (!pFile) {
        return 0;
    }
    return pFile->read(buffer, length);
}

static int32_t pngSeek(PNGFILE *handle, int32_t position)
{
    File *pFile = static_cast<File *>(handle->fHandle);
    if (!pFile) {
        return 0;
    }
    return pFile->seek(position);
}

static int PNGDraw(PNGDRAW *pDraw)
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

static bool displayPng(const char *path)
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

    epd_clear_screen();
    epd_clear();

    if (displayPng("/assets/supercon-x-splash.png")) {
        Serial.println("Displayed PNG");
    } else {
        Serial.println("Displaying fallback text");
        epd_fill(EPD_WHITE);
        epd_text(40, 40, "HS2026", EPD_BLACK, 6);
    }

    refresh_display(EPD_SLOW);
}

void loop()
{
    delay(1000);
}

