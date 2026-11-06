#ifndef PINS_H
#define PINS_H

/* Pin definitions for HS2026 badge (ESP32-S3) */
/* ------------------------------------------------- */
/* General purpose I/O */
#define USER_LED          1   // User LED

/* SAO (Skill Attoboard Option) headers */
#define SAO1_GPIO1        2
#define SAO2_GPIO1        4
#define SAO2_GPIO2        5
#define SAO3_GPIO2        6
#define SAO3_GPIO1        7
#define SAO1_GPIO2       42

/* Touch pads */
#define TP8               9
#define TP9              41

/* Buttons */
#define USER_A           39
#define USER_B           16
#define MENU_PLUS        15
#define MENU_MINUS       40

/* Battery sense */
#define BATT_SENSE       14

/* E‑ink display (Pervasive Displays E2969JS0B1, IL0373 controller) */
/* Raw EDP pins from badge schematic */
#define EDP_CS2          3   // Chip select 2 (not used for main display)
#define EDP_BUSY2        8   // Busy 2 (not used)
#define EDP_CS1         13   // Chip select 1
#define EDP_DC          10   // Data/Command
#define EDP_RST         11   // Reset
#define EDP_BUSY1       12   // Busy
#define EDP_SCLK        21   // SPI Clock
#define EDP_MISO        45   // SPI Master In Slave Out (not used)
#define EDP_MOSI        47   // SPI Master Out Slave In
#define EDP_PWR_EN      48   // Display power enable

/* Derived e‑ink pin names for clarity in code */
#define EINK_CS         EDP_CS1
#define EINK_DC         EDP_DC
#define EINK_RST        EDP_RST
#define EINK_BUSY       EDP_BUSY1
#define EINK_SCLK       EDP_SCLK
#define EINK_MOSI       EDP_MOSI
#define EINK_MISO       EDP_MISO
#define EINK_PWR        EDP_PWR_EN

/* NeoPixel bar */
#define NEOPIXEL_BAR    38

/* I2C for SAO (if needed) */
#define SAO_SDA         17
#define SAO_SCL         18

/* Display dimensions (for e‑ink) */
#define EINK_WIDTH      250
#define EINK_HEIGHT     122

#endif // PINS_H