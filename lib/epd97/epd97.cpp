#include "epd97.h"

#include <stdarg.h>
#include <stddef.h>

extern "C" {

bool epd_begin(void) {
  return true;
}

bool refresh_display(epd_refresh_t mode) {
  (void)mode;
  return true;
}

bool epd_clear_screen(void) {
  return true;
}

void epd_sleep(void) {
}

void epd_deep_sleep_prepare(void) {
}

void epd_set_keep_awake(bool on) {
  (void)on;
}

bool epd_keep_awake(void) {
  return false;
}

epd_panel_t epd_panel(void) {
  return EPD_PANEL_UNKNOWN;
}

const char* epd_panel_name(void) {
  return "stub";
}

uint32_t epd_last_panel_ms(void) {
  return 0;
}

uint32_t epd_last_frame_ms(void) {
  return 0;
}

uint8_t* epd_plane_bw(void) {
  return nullptr;
}

uint8_t* epd_plane_red(void) {
  return nullptr;
}

void epd_clear(void) {
}

void epd_fill(epd_color_t c) {
  (void)c;
}

void epd_pixel(int x, int y, epd_color_t c) {
  (void)x;
  (void)y;
  (void)c;
}

void epd_fill_rect(int x, int y, int w, int h, epd_color_t c) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)c;
}

void epd_rect(int x, int y, int w, int h, int thickness, epd_color_t c) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)thickness;
  (void)c;
}

void epd_hline(int x, int y, int w, int thickness, epd_color_t c) {
  (void)x;
  (void)y;
  (void)w;
  (void)thickness;
  (void)c;
}

void epd_vline(int x, int y, int h, int thickness, epd_color_t c) {
  (void)x;
  (void)y;
  (void)h;
  (void)thickness;
  (void)c;
}

int epd_text(int x, int y, const char* s, epd_color_t c, int scale) {
  (void)x;
  (void)y;
  (void)s;
  (void)c;
  (void)scale;
  return x;
}

int epd_printf(int x, int y, epd_color_t c, int scale, const char* fmt, ...) {
  (void)x;
  (void)y;
  (void)c;
  (void)scale;
  (void)fmt;
  return x;
}

int epd_text_width(const char* s, int scale) {
  (void)s;
  (void)scale;
  return 0;
}

}  // extern "C"
