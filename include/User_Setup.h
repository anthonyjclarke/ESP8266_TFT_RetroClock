// TFT_eSPI User Setup for ESP8266 TFT LED Retro Clock
// This file configures TFT_eSPI for the ILI9341/ST7789 display

#define USER_SETUP_ID 200

// Driver selection
#define ILI9341_DRIVER      // Use ILI9341 driver

// ESP8266 Pin Configuration
// D1 Mini Pro pin mapping
#define TFT_CS   PIN_D1  // Chip select control pin D1 (GPIO5)
#define TFT_DC   PIN_D2  // Data Command control pin D2 (GPIO4)
#define TFT_RST  -1      // Set to -1 if using ESP reset (connected to ESP RST or 3.3V)

// Hardware SPI on ESP8266 uses:
// MOSI (D7) = GPIO13
// MISO (D6) = GPIO12 (not used for display)
// SCK  (D5) = GPIO14

// Fonts
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

// SPI frequency
#define SPI_FREQUENCY  40000000  // 40 MHz for ESP8266
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
