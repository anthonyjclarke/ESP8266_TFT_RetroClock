#include "Arduino.h"

/*
 * ESP8266 TFT Matrix Clock - TFT Edition
 * Author: Refactored from LED Matrix version by Anthony Clarke
 * Display: 1.8/2.4/2.8 Inch SPI TFT LCD (ILI9341/ST7789)
 *
 * This version simulates the LED matrix appearance on a TFT display
 * All functionality remains identical to the original LED matrix version
 *
 * ======================== CHANGELOG ========================
 * 20th January 2026 - Version 1.1:
 *   - Added about section in web UI footer with GitHub repo link and BlueSky contact
 *   - Created CHANGELOG.md file following semantic versioning
 *   - Added VERSION constant for centralized version management
 *   - Increased TFT Display Mirror size from 320x164 to 800x410 pixels (25px per LED)
 *   - Added inline comments for easy LED size customization
 *
 * 19th December 2025 - Version 1.0 (formerly 2.2):
 *   - Added TFT Display Mirror feature on web page (real-time Canvas rendering)
 *   - New /api/display endpoint returns 64-byte buffer with minimal overhead (~200 bytes/request)
 *   - Canvas-based LED matrix rendering in browser (supports both display styles)
 *   - Auto-refresh every 500ms for near real-time display mirroring
 *   - Dynamically syncs LED color, surround color, and display style settings
 *
 * 18th December 2025 - Version 2.1:
 *   - Fixed Mode 2 (Time+Date) to remove leading zero from single-digit hours
 *   - Completely redesigned web interface with modern dark theme
 *   - Added large digital clock display with live auto-update (updates every second)
 *   - Implemented dynamic temperature icons based on actual temperature readings:
 *     * 🔥 Hot (≥30°C), ☀️ Warm (25-29°C), 🌤️ Pleasant (20-24°C)
 *     * ⛅ Mild (15-19°C), ☁️ Cool (10-14°C), 🌧️ Cold (5-9°C), ❄️ Freezing (<5°C)
 *   - Added dynamic humidity icons (💦 high, 💧 normal, 🏜️ low)
 *   - Enhanced environment display with color-coded values and glowing effects
 *   - Implemented fully responsive web design for mobile/tablet/desktop
 *   - Fixed LED surround color rendering - now properly visible and configurable
 *   - Improved LED pixel rendering with better surround ring visibility
 *   - Added fluid typography with CSS clamp() for all screen sizes
 *   - Created responsive grid layout for environment cards
 *   - Added touch-friendly UI elements and hover effects
 *   - Migrated from Adafruit_GFX/Adafruit_ILI9341 to TFT_eSPI library
 *   - Improved rendering performance with hardware-optimized library
 *   - Configured 40MHz SPI bus speed for faster display updates
 *
 * =========================== TODO ==========================
 * - Get weather from online API and display on matrix and webpage
 * - Add OTA firmware update capability
 * - add mew display modes, like morphing (from @cbmamiga) 
 * - refactor code for ESP32 compatibility and use of the CYD display - https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
 * - Update Readme.md with sample screenshots of new web interface and TFT display
 * - Optimize TFT drawing routines further for even faster refresh rates
 * 
 * ======================== FEATURES ========================
 * - Simulates 4x2 MAX7219 LED matrix appearance on TFT display
 * - WiFiManager for easy WiFi setup (no hardcoded credentials)
 * - BME280 I2C temperature/pressure/humidity sensor (optional)
 * - Automatic NTP time synchronization with DST support
 * - Modern responsive web interface with live updates
 * - Realistic LED rendering with customizable colors
 * - Dynamic environment icons based on sensor readings
 * - Multiple timezone support with POSIX TZ strings
 * - Two display styles: Default (blocks) and Realistic (circular LEDs)
 * - Mobile-responsive design with adaptive layouts
 */

// ======================== LIBRARIES ========================
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <time.h>
#include <TZ.h>
#include <TFT_eSPI.h>  // Hardware-specific library with optimized performance

// ======================== VERSION ========================
const char* VERSION = "1.1.0";

// ======================== PIN DEFINITIONS ========================
// TFT Display SPI Pins - Now configured in User_Setup.h for TFT_eSPI
#define LED_PIN   D8    // TFT Backlight control

// Sensor Pins (BME280 only)
#define SDA_PIN   D4    // I2C Data (BME280)
#define SCL_PIN   D3    // I2C Clock (BME280)

// ======================== DISPLAY CONFIGURATION ========================
#define NUM_MAX           8      // Simulated number of 8x8 LED matrices (2 rows × 4 columns)
#define MATRIX_WIDTH      8      // Width of each simulated matrix
#define MATRIX_HEIGHT     8      // Height of each simulated matrix
#define LINE_WIDTH        32     // Display width in pixels (4 matrices wide)
#define DISPLAY_ROWS      2      // Number of rows of matrices
#define ROTATE            90     // Display rotation to match original
#define LED_SIZE          10     // Size of each simulated LED pixel (maximized for 320px width)
#define LED_SPACING       0      // No spacing between LEDs
#define TOTAL_WIDTH       32     // Total width: 32 pixels
#define TOTAL_HEIGHT      16     // Total height: 16 pixels (2 rows of 8)
#define LED_COLOR         0xF800 // Red color for LEDs (RGB565) - FULL BRIGHTNESS
#define BG_COLOR          0x0000 // Black background
#define LED_OFF_COLOR     0x2000 // Slightly brighter dark red for "off" LEDs (was 0x1082)

// ======================== DISPLAY STYLE CONFIGURATION ========================
// Display styles: 0 = Default (solid blocks), 1 = Realistic (circular LEDs)
#define DEFAULT_DISPLAY_STYLE 1  // Start with realistic style

// Color presets (RGB565 format)
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_WHITE       0xFFFF
#define COLOR_ORANGE      0xFD20
#define COLOR_DARK_GRAY   0x7BEF
#define COLOR_LIGHT_GRAY  0xC618
#define COLOR_BLACK       0x0000

// Calculate display dimensions
// When LED_SPACING = 0, formula simplifies to: LED_SIZE * count
// Plus 4-pixel gap between the two matrix rows (authentic spacing)
#define DISPLAY_WIDTH     (LED_SIZE * TOTAL_WIDTH)
#define DISPLAY_HEIGHT    (LED_SIZE * TOTAL_HEIGHT + 4)  // +4 for authentic row gap

// ======================== TIMING CONFIGURATION ========================
#define SENSOR_UPDATE_INTERVAL       60000  // Update sensor every 60s
#define NTP_SYNC_INTERVAL            3600000 // Sync NTP every hour
#define STATUS_PRINT_INTERVAL        10000  // Print status every 10s

// ======================== DEBUG CONFIGURATION ========================
#define DEBUG_ENABLED 1

// ======================== DISPLAY OPTIMIZATION ========================
#define BRIGHTNESS_BOOST 1  // Set to 1 for maximum brightness (ignores brightness level)
#define FAST_REFRESH 1      // Set to 1 to only redraw changed pixels (much faster)

#if DEBUG_ENABLED
  #define DEBUG(x) x
#else
  #define DEBUG(x)
#endif

// ======================== DISPLAY OBJECT ========================
TFT_eSPI tft = TFT_eSPI();  // TFT_eSPI uses configuration from User_Setup.h

// ======================== DISPLAY BUFFER ========================
// Virtual screen buffer matching original LED matrix structure
// Buffer is organized as: scr[x + y * LINE_WIDTH] where each byte = 8 vertical pixels
// With 8 matrices (2 rows × 4 columns): 32 pixels wide × 16 pixels tall
byte scr[LINE_WIDTH * DISPLAY_ROWS]; // 32 columns × 2 rows = 64 bytes

// ======================== GLOBAL OBJECTS ========================
ESP8266WebServer server(80);
WiFiManager wifiManager;
Adafruit_BME280 bme280;

// ======================== FONT INCLUDES ========================
#include "fonts.h"
#include "timezones.h"

// ======================== TIME VARIABLES ========================
int hours = 0, minutes = 0, seconds = 0;
int hours24 = 0;  // 24-hour format
int day = 1, month = 1, year = 2025;
int lastSecond = -1;
bool use24HourFormat = false;  // Default to 12-hour format

// ======================== SENSOR VARIABLES ========================
bool sensorAvailable = false;
int temperature = 0;
int humidity = 0;
int pressure = 0;
bool useFahrenheit = false;

// ======================== TIMING VARIABLES ========================
unsigned long lastSensorUpdate = 0;
unsigned long lastNTPSync = 0;
unsigned long lastStatusPrint = 0;

// ======================== DISPLAY STYLE VARIABLES ========================
int displayStyle = DEFAULT_DISPLAY_STYLE;  // 0=Default, 1=Realistic
uint16_t ledOnColor = COLOR_RED;           // Color for lit LEDs
uint16_t ledSurroundColor = COLOR_DARK_GRAY; // Dark gray for authentic MAX7219 look
uint16_t ledOffColor = 0x2000;             // Color for unlit LEDs (dim)
bool surroundMatchesLED = false;           // Track if surround should match LED color
bool forceFullRedraw = false;              // Flag to force immediate complete redraw

// ======================== DISPLAY MODES ========================
int currentMode = 0; // 0=Time+Temp, 1=Time Large, 2=Time+Date
unsigned long lastModeSwitch = 0;
#define MODE_SWITCH_INTERVAL 5000

// ======================== TIMEZONE ========================
// Timezone definitions are now in include/timezones.h
int currentTimezone = 0;

// ======================== TFT DISPLAY FUNCTIONS ========================

void initTFT() {
  DEBUG(Serial.println("Initializing TFT Display..."));

  // Setup backlight pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // Turn backlight ON
  DEBUG(Serial.println("Backlight enabled"));

  // Add small delay before TFT initialization
  delay(100);

  // TFT_eSPI initialization
  tft.init();
  tft.setRotation(3);  // Rotation 3 = landscape mode (320x240)
  DEBUG(Serial.printf("TFT_eSPI initialized, rotation set to 3\n"));

  // Small delay after initialization
  delay(100);

  // Check actual dimensions
  DEBUG(Serial.printf("TFT reports dimensions: %d x %d\n", tft.width(), tft.height()));

  tft.fillScreen(BG_COLOR);

  // Calculate display dimensions
  int displayWidth = tft.width();
  int displayHeight = tft.height();
  int offsetX = ((displayWidth - DISPLAY_WIDTH) / 2) > 0 ? ((displayWidth - DISPLAY_WIDTH) / 2) : 0;
  int offsetY = ((displayHeight - DISPLAY_HEIGHT) / 2) > 0 ? ((displayHeight - DISPLAY_HEIGHT) / 2) : 0;
  
  DEBUG(Serial.printf("TFT Display initialized: %dx%d\n", displayWidth, displayHeight));
  DEBUG(Serial.printf("LED Matrix area: %dx%d at offset (%d,%d)\n", 
        DISPLAY_WIDTH, DISPLAY_HEIGHT, offsetX, offsetY));
  
  // Verify we have valid dimensions
  if (displayWidth <= 0 || displayHeight <= 0) {
    DEBUG(Serial.println("ERROR: Invalid TFT dimensions!"));
    DEBUG(Serial.println("Check TFT wiring and display type selection"));
  }
}

void clearScreen() {
  for (int i = 0; i < LINE_WIDTH * DISPLAY_ROWS; i++) {
    scr[i] = 0;
  }
}

// Dim an RGB565 color while preserving hue
uint16_t dimRGB565(uint16_t color, int factor) {
  // Extract RGB components from RGB565
  int r = (color >> 11) & 0x1F;  // 5 bits
  int g = (color >> 5) & 0x3F;   // 6 bits
  int b = color & 0x1F;          // 5 bits
  
  // Dim each component by factor (1=50%, 2=33%, 3=25%, etc)
  r = r / (factor + 1);
  g = g / (factor + 1);
  b = b / (factor + 1);
  
  // Recombine into RGB565
  return (r << 11) | (g << 5) | b;
}

// Force a complete refresh by resetting FAST_REFRESH tracking
void forceCompleteRefresh() {
  // This will be called by refreshAll to reset its static state
  tft.fillScreen(BG_COLOR);
  clearScreen();
}

void drawLEDPixel(int x, int y, bool lit) {
  // Bounds checking
  if (x < 0 || x >= TOTAL_WIDTH || y < 0 || y >= TOTAL_HEIGHT) {
    return;
  }
  
  // Calculate screen position with centering offset
  int offsetX = ((tft.width() - DISPLAY_WIDTH) / 2) > 0 ? ((tft.width() - DISPLAY_WIDTH) / 2) : 0;
  int offsetY = ((tft.height() - DISPLAY_HEIGHT) / 2) > 0 ? ((tft.height() - DISPLAY_HEIGHT) / 2) : 0;
  
  // Add extra gap between matrix rows (after row 7, before row 8)
  // 4-pixel gap for authentic MAX7219 hardware spacing
  int matrixGap = (y >= 8) ? 4 : 0;  // 4-pixel gap between matrix rows
  
  int screenX = offsetX + x * LED_SIZE;
  int screenY = offsetY + y * LED_SIZE + matrixGap;
  
  if (displayStyle == 0) {
    // ========== DEFAULT STYLE: Solid square blocks ==========
    uint16_t color = lit ? ledOnColor : BG_COLOR;  // Off LEDs are BLACK
    tft.fillRect(screenX, screenY, LED_SIZE, LED_SIZE, color);
  } 
  else {
    // ========== REALISTIC STYLE: Circular LED with surround ==========
    // Enhanced for authenticity matching real MAX7219 hardware
    
    if (!lit) {
      // OFF LED: Show dark circle (visible but dim, like real hardware)
      // Real LEDs are visible even when off - dark red/gray circle
      
      // Fill background black
      tft.fillRect(screenX, screenY, LED_SIZE, LED_SIZE, BG_COLOR);
      
      // Draw subtle dark circle for off LED (visible against black)
      // Using darker version of surround color for off LED housing
      uint16_t offHousing = dimRGB565(ledSurroundColor, 7);  // Very dim (1/8 brightness)
      uint16_t offLED = 0x1800;  // Very dark red (barely visible)
      
      for (int py = 1; py < 9; py++) {
        for (int px = 1; px < 9; px++) {
          int cx = px - 1;
          int cy = py - 1;
          int dx = (cx * 2 - 7);
          int dy = (cy * 2 - 7);
          int distSq = dx * dx + dy * dy;
          
          if (distSq <= 42) {  // Inner dark circle
            tft.drawPixel(screenX + px, screenY + py, offLED);
          }
          else if (distSq <= 58) {  // Dim surround
            tft.drawPixel(screenX + px, screenY + py, offHousing);
          }
        }
      }
    }
    else {
      // LIT LED: Draw bright circular LED with surround
      // Fill background with surround color first
      tft.fillRect(screenX, screenY, LED_SIZE, LED_SIZE, ledSurroundColor);

      // Draw from outside in for better circular appearance
      for (int py = 0; py < 10; py++) {
        for (int px = 0; px < 10; px++) {
          int cx = px;
          int cy = py;
          int dx = (cx * 2 - 9);
          int dy = (cy * 2 - 9);
          int distSq = dx * dx + dy * dy;

          uint16_t pixelColor;

          // Redesigned for better visibility of surround
          if (distSq <= 18) {
            // Bright center (core)
            pixelColor = ledOnColor;
          }
          else if (distSq <= 38) {
            // Main LED body (still bright)
            pixelColor = ledOnColor;
          }
          else if (distSq <= 62) {
            // Surround/bezel ring (use full surround color, not dimmed)
            pixelColor = ledSurroundColor;
          }
          else {
            // Outside circle: black
            pixelColor = BG_COLOR;
          }

          tft.drawPixel(screenX + px, screenY + py, pixelColor);
        }
      }
    }
  }
}

void refreshAll() {
  // The buffer is organized as scr[x + y * LINE_WIDTH] where each byte = 8 vertical pixels
  // We have 2 rows of matrices, so we need to handle 16 pixels vertically
  
  #if FAST_REFRESH
    // Static buffer to track previous state for change detection
    static byte lastScr[LINE_WIDTH * DISPLAY_ROWS] = {0};
    static bool firstRun = true;
    
    // Check if external force refresh was requested
    if (forceFullRedraw) {
      // Clear the tracking buffer to force everything to redraw
      for (int i = 0; i < LINE_WIDTH * DISPLAY_ROWS; i++) {
        lastScr[i] = 0xFF;  // Set to invalid state
      }
      forceFullRedraw = false;  // Clear the flag
      firstRun = true;  // Treat as first run
      DEBUG(Serial.println("FAST_REFRESH cache cleared - forcing full redraw"));
    }
  #endif
  
  // Process both rows of matrices (0-7 pixels and 8-15 pixels)
  for (int row = 0; row < DISPLAY_ROWS; row++) {
    for (int displayX = 0; displayX < LINE_WIDTH; displayX++) {
      int bufferIndex = displayX + row * LINE_WIDTH;
      
      if (bufferIndex >= 0 && bufferIndex < LINE_WIDTH * DISPLAY_ROWS) {
        byte pixelByte = scr[bufferIndex];
        
        #if FAST_REFRESH
          // Only redraw if changed or first run
          if (firstRun || pixelByte != lastScr[bufferIndex]) {
            lastScr[bufferIndex] = pixelByte;
        #endif
            
            // Each bit in pixelByte represents a vertical pixel (0-7)
            for (int bitPos = 0; bitPos < 8; bitPos++) {
              int displayY = row * 8 + bitPos;  // Calculate actual Y position (0-15)
              bool lit = (pixelByte & (1 << bitPos)) != 0;
              
              drawLEDPixel(displayX, displayY, lit);
            }
            
        #if FAST_REFRESH
          }
        #endif
      }
    }
  }
  
  #if FAST_REFRESH
    firstRun = false;
  #endif
}

void invert() {
  for (int i = 0; i < LINE_WIDTH * DISPLAY_ROWS; i++) {
    scr[i] = ~scr[i];
  }
}

void scrollLeft() {
  for (int i = 0; i < LINE_WIDTH * DISPLAY_ROWS - 1; i++) {
    scr[i] = scr[i + 1];
  }
  scr[LINE_WIDTH * DISPLAY_ROWS - 1] = 0;
}

// ======================== FONT HELPER FUNCTIONS ========================

int charWidth(char c, const uint8_t* font) {
  int firstChar = pgm_read_byte(font + 2);
  int lastChar = pgm_read_byte(font + 3);
  
  if (c < firstChar || c > lastChar) return 0;
  
  int charIndex = c - firstChar;
  int offset = 4;
  for (int i = 0; i < charIndex; i++) {
    int w = pgm_read_byte(font + offset);
    offset += w * pgm_read_byte(font + 1) + 1;
  }
  
  return pgm_read_byte(font + offset);
}

// Forward declaration
int drawCharWithY(int x, int yPos, char c, const uint8_t* font);

int drawChar(int x, char c, const uint8_t* font) {
  return drawCharWithY(x, 0, c, font);  // Default to yPos=0 (top row)
}

int drawCharWithY(int x, int yPos, char c, const uint8_t* font) {
  int fwd = pgm_read_byte(font);
  int fht = pgm_read_byte(font + 1);
  int offs = pgm_read_byte(font + 2);
  int last = pgm_read_byte(font + 3);
  
  if (c < offs || c > last) return 0;
  
  c -= offs;
  int fht8 = (fht + 7) / 8;
  font += 4 + c * (fht8 * fwd + 1);
  
  int j, i, w = pgm_read_byte(font);
  
  for (j = 0; j < fht8; j++) {
    for (i = 0; i < w; i++) {
      if (x + i >= 0 && x + i < LINE_WIDTH) {
        int bufferIndex = x + LINE_WIDTH * (j + yPos) + i;
        if (bufferIndex >= 0 && bufferIndex < LINE_WIDTH * DISPLAY_ROWS) {
          scr[bufferIndex] = pgm_read_byte(font + 1 + fht8 * i + j);
        }
      }
    }
    if (x + i < LINE_WIDTH && x + i >= 0) {
      int bufferIndex = x + LINE_WIDTH * (j + yPos) + i;
      if (bufferIndex >= 0 && bufferIndex < LINE_WIDTH * DISPLAY_ROWS) {
        scr[bufferIndex] = 0;
      }
    }
  }
  
  return w;
}

int stringWidth(const char* str, const uint8_t* font) {
  int width = 0;
  while (*str) {
    width += charWidth(*str++, font) + 1;
  }
  return width - 1;
}

void showMessage(const char* msg) {
  if (msg == NULL || strlen(msg) == 0) return;
  
  clearScreen();
  delay(10); // Small delay after clearing
  
  int width = stringWidth(msg, font3x7);
  int x = (TOTAL_WIDTH - width) / 2;
  
  // Ensure x is within valid range
  if (x < 0) x = 0;
  if (x >= TOTAL_WIDTH) x = TOTAL_WIDTH - 1;
  
  while (*msg) {
    x += drawChar(x, *msg++, font3x7) + 1;
  }
  
  delay(10); // Small delay before refresh
  refreshAll();
}

// ======================== DISPLAY FUNCTIONS ========================

void displayTimeAndTemp() {
  clearScreen();
  
  char buf[32];
  bool showDots = (seconds % 2) == 0;  // Blink colon every second
  
  // Top row (yPos = 0): Time - optimized to fit in 32 pixels
  // Use tighter spacing to fit everything
  int x = 0;  // Start at very left edge
  
  // Determine display hours and whether to show seconds
  int displayHours = use24HourFormat ? hours24 : hours;
  bool canShowSeconds = true;  // Assume we can show seconds
  
  // In 24-hour mode with hours >= 10, we need more space
  // Example: "23:45:12" needs more pixels than "9:45:12"
  if (use24HourFormat && hours24 >= 10) {
    canShowSeconds = false;  // Not enough room for seconds in 24-hour mode
  }
  
  // Hours (1-2 digits)
  sprintf(buf, "%d", displayHours);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x8rn);
    if (*(p+1)) x++;  // Add spacing only between digits (saves space)
  }
  
  // Colon (or space if not showing)
  if (showDots) {
    x += drawCharWithY(x, 0, ':', digits5x8rn);
    x += 1;  // Spacing after colon when showing
  } else {
    x += 2;  // Reserve colon width when hidden (NOT 6 - that's too much!)
  }
  
  // Minutes (always 2 digits)
  sprintf(buf, "%02d", minutes);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x8rn);
    if (*(p+1)) x++;  // Add spacing only between digits (saves space)
  }
  
  // Seconds (2 digits in smaller font) - only if there's room
  if (canShowSeconds) {
    // Small gap before seconds
    x++;
    
    sprintf(buf, "%02d", seconds);
    if (x + 7 <= LINE_WIDTH) {  // Check if seconds will fit (3px*2 + 1 spacing = 7px)
      for (const char* p = buf; *p; p++) {
        if (x < LINE_WIDTH - 3) {  // Ensure char fits
          x += drawCharWithY(x, 0, *p, digits3x5);
          if (*(p+1) && x < LINE_WIDTH) x++;  // Add spacing if room
        }
      }
    }
  }
  
  // Bottom row (yPos = 1): Temperature and Humidity
  x = 0;  // Start at left
  if (sensorAvailable) {
    int displayTemp = useFahrenheit ? (temperature * 9 / 5 + 32) : temperature;
    char tempUnit = useFahrenheit ? 'F' : 'C';
    sprintf(buf, "T%d%c H%d%%", displayTemp, tempUnit, humidity);
  } else {
    sprintf(buf, "NO SENSOR");
  }
  
  for (const char* p = buf; *p; p++) {
    if (x < LINE_WIDTH - 3) {  // Ensure character fits
      x += drawCharWithY(x, 1, *p, font3x7);
      if (*(p+1) && x < LINE_WIDTH) x++;
    }
  }
  
  // NOTE: Bottom line shift disabled - causes visual artifacts on TFT
  // This was from original MAX7219 code but looks wrong on TFT display
  // Uncomment if you want the original behavior:
  // for (int i = 0; i < LINE_WIDTH; i++) {
  //   scr[LINE_WIDTH + i] <<= 1;
  // }
}

void displayTimeLarge() {
  clearScreen();
  
  char buf[32];
  bool showDots = (seconds % 2) == 0;
  
  // Determine display hours based on format
  int displayHours = use24HourFormat ? hours24 : hours;
  
  // Large time display on top row using 16-pixel tall font
  // Start position depends on whether hours is 1 or 2 digits
  int x = (displayHours > 9) ? 0 : 3;
  
  // Draw hours
  sprintf(buf, "%d", displayHours);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x16rn);
    if (*(p+1)) x++;  // Spacing between hour digits only
  }
  
  // Draw colon - reserve same total space whether showing or not
  if (showDots) {
    x += drawCharWithY(x, 0, ':', digits5x16rn);
    // No spacing after colon in large mode (tight layout)
  } else {
    x += 1;  // Reserve colon width when not showing
  }
  
  // Draw minutes - NO extra spacing before, tight to colon
  sprintf(buf, "%02d", minutes);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x16rn);
    if (*(p+1)) x++;  // Spacing between minute digits only
  }
  
  // Add small gap before seconds
  x++;
  
  // Draw seconds in small font
  sprintf(buf, "%02d", seconds);
  for (const char* p = buf; *p; p++) {
    if (x < LINE_WIDTH - 3) {  // Check if room remains
      x += drawCharWithY(x, 0, *p, font3x7);
      if (*(p+1) && x < LINE_WIDTH - 3) x++;  // Spacing between second digits if room
    }
  }
}

void displayTimeAndDate() {
  clearScreen();
  
  char buf[32];
  bool showDots = (seconds % 2) == 0;
  
  // Determine display hours based on format
  int displayHours = use24HourFormat ? hours24 : hours;
  
  // Top row: Time
  int x = 0;
  sprintf(buf, "%d", displayHours);  // No leading zero
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x8rn);
    if (*(p+1)) x++;  // Spacing only between digits (saves space)
  }
  
  if (showDots) {
    x += drawCharWithY(x, 0, ':', digits5x8rn);
    x += 1;  // Spacing after colon when showing
  } else {
    x += 2;  // Reserve colon width when hidden
  }
  
  sprintf(buf, "%02d", minutes);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x8rn);
    if (*(p+1)) x++;  // Spacing only between digits (saves space)
  }
  
  // Add seconds in small font
  x++;  // Small gap before seconds
  sprintf(buf, "%02d", seconds);
  for (const char* p = buf; *p; p++) {
    if (x < LINE_WIDTH - 3) {  // Check if room remains
      x += drawCharWithY(x, 0, *p, digits3x5);
      if (*(p+1) && x < LINE_WIDTH - 3) x++;  // Spacing between second digits if room
    }
  }
  
  // Bottom row: Date
  x = 2;
  sprintf(buf, "%02d/%02d/%02d", day, month, year % 100);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 1, *p, font3x7) + 1;
  }
  
  // NOTE: Bottom line shift disabled - causes visual artifacts on TFT
  // for (int i = 0; i < LINE_WIDTH; i++) {
  //   scr[LINE_WIDTH + i] <<= 1;
  // }
}

// ======================== SENSOR FUNCTIONS ========================

bool testSensor() {
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (!bme280.begin(0x76, &Wire)) {
    DEBUG(Serial.println("BME280 sensor not found at 0x76"));
    if (!bme280.begin(0x77, &Wire)) {
      DEBUG(Serial.println("BME280 sensor not found at 0x77 either"));
      return false;
    }
  }
  
  bme280.setSampling(Adafruit_BME280::MODE_FORCED,
                     Adafruit_BME280::SAMPLING_X1,
                     Adafruit_BME280::SAMPLING_X1,
                     Adafruit_BME280::SAMPLING_X1,
                     Adafruit_BME280::FILTER_OFF);
  
  float temp = bme280.readTemperature();
  float hum = bme280.readHumidity();
  
  if (isnan(temp) || isnan(hum) || temp < -50 || temp > 100 || hum < 0 || hum > 100) {
    DEBUG(Serial.println("BME280 readings invalid"));
    return false;
  }
  
  DEBUG(Serial.printf("BME280 OK: %.1f°C, %.1f%%\n", temp, hum));
  return true;
}

void updateSensorData() {
  if (!sensorAvailable) return;
  
  bme280.takeForcedMeasurement();
  float temp = bme280.readTemperature();
  float hum = bme280.readHumidity();
  float pres = bme280.readPressure() / 100.0F;
  
  if (!isnan(temp) && temp >= -50 && temp <= 100) {
    temperature = (int)round(temp);
  }
  
  if (!isnan(hum) && hum >= 0 && hum <= 100) {
    humidity = (int)round(hum);
  }
  
  if (!isnan(pres) && pres >= 800 && pres <= 1200) {
    pressure = (int)round(pres);
  }
}

// ======================== NTP SYNC FUNCTION ========================

void syncNTP() {
  DEBUG(Serial.println("Syncing time with NTP..."));
  
  configTime(timezones[currentTimezone].tzString, "pool.ntp.org", "time.nist.gov");
  
  time_t now = time(nullptr);
  int attempts = 0;
  while (now < 24 * 3600 && attempts < 20) {
    delay(500);
    now = time(nullptr);
    attempts++;
  }
  
  if (now > 24 * 3600) {
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    hours = timeinfo.tm_hour % 12;
    if (hours == 0) hours = 12;
    hours24 = timeinfo.tm_hour;
    minutes = timeinfo.tm_min;
    seconds = timeinfo.tm_sec;
    day = timeinfo.tm_mday;
    month = timeinfo.tm_mon + 1;
    year = timeinfo.tm_year + 1900;
    
    DEBUG(Serial.printf("Time synced: %02d:%02d:%02d %02d/%02d/%d (TZ: %s)\n",
                        hours24, minutes, seconds, day, month, year,
                        timezones[currentTimezone].name));
  } else {
    DEBUG(Serial.println("NTP sync failed"));
  }
}

// ======================== TIME UPDATE FUNCTION ========================

void updateTime() {
  time_t now = time(nullptr);
  if (now < 24 * 3600) return;
  
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  
  hours = timeinfo.tm_hour % 12;
  if (hours == 0) hours = 12;
  hours24 = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
  day = timeinfo.tm_mday;
  month = timeinfo.tm_mon + 1;
  year = timeinfo.tm_year + 1900;
  
  if (seconds != lastSecond) {
    lastSecond = seconds;
    DEBUG(Serial.printf("Display update - Mode: %d, Time: %02d:%02d:%02d\n", currentMode, hours24, minutes, seconds));
    switch (currentMode) {
      case 0: displayTimeAndTemp(); break;
      case 1: displayTimeLarge(); break;
      case 2: displayTimeAndDate(); break;
    }
    refreshAll();
  }
  
  // Auto-switch modes
  if (millis() - lastModeSwitch > MODE_SWITCH_INTERVAL) {
    currentMode = (currentMode + 1) % 3;
    lastModeSwitch = millis();
  }
}

// ======================== WEB SERVER FUNCTIONS ========================

void setupWebServer() {
  // Root page handler
  server.on("/", []() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>TFT LED Clock</title>";
    html += "<style>";
    html += "*{box-sizing:border-box;}";
    html += "body{font-family:'Segoe UI',Arial,sans-serif;margin:0;padding:15px;background:#1a1a1a;color:#fff;max-width:1200px;margin:0 auto;}";
    html += ".header{text-align:center;margin-bottom:20px;}";
    html += "h1{color:#fff;font-size:clamp(20px,5vw,28px);font-weight:600;margin:0 0 30px 0;}";
    html += ".time-display{background:linear-gradient(135deg,#2a2a2a,#1e1e1e);padding:clamp(20px,5vw,40px);border-radius:15px;box-shadow:0 8px 32px rgba(0,0,0,0.3);margin-bottom:20px;}";
    html += ".time-display h2{color:#aaa;font-size:clamp(16px,4vw,20px);font-weight:400;margin:0 0 15px 0;text-align:left;}";
    html += ".clock{font-size:clamp(48px,15vw,120px);font-weight:700;text-align:center;margin:15px 0;font-family:'Courier New',monospace;color:#7CFC00;text-shadow:0 0 30px rgba(124,252,0,0.5);line-height:1.1;}";
    html += ".date{font-size:clamp(24px,7vw,48px);font-weight:600;text-align:center;margin:15px 0;font-family:'Courier New',monospace;color:#4A90E2;text-shadow:0 0 20px rgba(74,144,226,0.5);line-height:1.2;}";
    html += ".environment{background:linear-gradient(135deg,#2a2a2a,#1e1e1e);padding:clamp(20px,4vw,40px);border-radius:15px;box-shadow:0 8px 32px rgba(0,0,0,0.3);margin-bottom:20px;}";
    html += ".environment p{margin:10px 0;}";
    html += ".env-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:clamp(15px,3vw,30px);text-align:center;}";
    html += ".env-item{padding:clamp(15px,3vw,20px);background:rgba(255,255,255,0.05);border-radius:10px;transition:transform 0.2s;}";
    html += ".env-item:hover{transform:translateY(-5px);background:rgba(255,255,255,0.08);}";
    html += ".env-icon{font-size:clamp(40px,10vw,60px);margin-bottom:8px;display:block;}";
    html += ".env-value{font-size:clamp(24px,6vw,36px);font-weight:700;margin:8px 0;font-family:'Courier New',monospace;line-height:1.2;}";
    html += ".env-label{font-size:clamp(12px,3vw,16px);color:#aaa;text-transform:uppercase;letter-spacing:1px;}";
    html += ".card{background:linear-gradient(135deg,#2a2a2a,#1e1e1e);padding:clamp(15px,3vw,20px);margin:10px 0;border-radius:10px;box-shadow:0 4px 16px rgba(0,0,0,0.3);}";
    html += "h2{color:#aaa;border-bottom:2px solid #4CAF50;padding-bottom:5px;font-size:clamp(16px,4vw,18px);font-weight:500;margin-top:0;}";
    html += "button{background:#4CAF50;color:white;border:none;padding:10px 15px;cursor:pointer;border-radius:5px;margin:5px;font-size:clamp(12px,3vw,14px);white-space:nowrap;}";
    html += "button:hover{background:#45a049;}";
    html += "select{padding:8px;font-size:clamp(12px,3vw,14px);background:#1e1e1e;color:#fff;border:1px solid #444;border-radius:5px;width:100%;max-width:300px;}";
    html += "p{color:#ccc;font-size:clamp(13px,3vw,15px);line-height:1.6;}";
    html += "@media(max-width:768px){";
    html += ".env-grid{grid-template-columns:1fr;}";
    html += ".clock{font-size:clamp(40px,12vw,80px);}";
    html += ".date{font-size:clamp(20px,6vw,36px);}";
    html += "body{padding:10px;}";
    html += ".time-display,.environment,.card{padding:15px;}";
    html += "}";
    html += "@media(min-width:769px) and (max-width:1024px){";
    html += ".env-grid{grid-template-columns:repeat(3,1fr);}";
    html += "}";
    // TFT Display Mirror styles
    html += ".tft-mirror{background:linear-gradient(135deg,#2a2a2a,#1e1e1e);padding:clamp(15px,3vw,25px);border-radius:15px;box-shadow:0 8px 32px rgba(0,0,0,0.3);margin-bottom:20px;text-align:center;}";
    html += ".tft-mirror h2{color:#aaa;border-bottom:2px solid #E91E63;padding-bottom:5px;font-size:clamp(16px,4vw,18px);font-weight:500;margin-top:0;text-align:left;}";
    html += ".canvas-container{display:flex;justify-content:center;align-items:center;padding:15px;background:#000;border-radius:10px;margin-top:15px;}";
    html += "#tftCanvas{image-rendering:pixelated;image-rendering:crisp-edges;border-radius:5px;max-width:100%;height:auto;}";
    html += ".tft-label{color:#888;font-size:12px;margin-top:10px;}";
    html += ".footer{text-align:center;padding:20px;margin-top:30px;border-top:1px solid #333;color:#888;font-size:clamp(12px,3vw,14px);}";
    html += ".footer p{margin:5px 0;}";
    html += ".footer a{color:#4CAF50;text-decoration:none;transition:color 0.3s;}";
    html += ".footer a:hover{color:#7CFC00;text-decoration:underline;}";
    html += "</style>";
    html += "<script>";
    html += "function updateTime(){";
    html += "fetch('/api/time')";
    html += ".then(function(r){return r.json();})";
    html += ".then(function(d){";
    html += "var clock=document.getElementById('clock');";
    html += "var date=document.getElementById('date');";
    html += "var h=d.hours;";
    html += "var ampm='';";
    html += "if(!d.use24hour){";
    html += "ampm=(h>=12)?' PM':' AM';";
    html += "h=(h%12)||12;";
    html += "}";
    html += "if(clock){clock.textContent=(d.use24hour&&h<10?'0':'')+h+':'+(d.minutes<10?'0':'')+d.minutes+':'+(d.seconds<10?'0':'')+d.seconds+ampm;}";
    html += "if(date){date.textContent=(d.day<10?'0':'')+d.day+'/'+(d.month<10?'0':'')+d.month+'/'+d.year;}";
    html += "})";
    html += ".catch(function(e){console.log('Update failed:',e);});";
    html += "}";
    html += "setInterval(updateTime,1000);";
    html += "setTimeout(updateTime,100);";
    // TFT Display Mirror - Canvas rendering functions
    // LED size controls the canvas display size on the web page
    // ledSize=25 creates 800x410px canvas (25px per LED × 32×16 LEDs)
    // Options: 10=320x164, 15=480x246, 20=640x328, 25=800x410, 30=960x492
    // gapSize should be ledSize × 0.4 for proper row spacing
    html += "var tftCanvas,tftCtx,ledSize=25,gapSize=10;";
    html += "function rgb565ToHex(c){var r=((c>>11)&0x1F)*8,g=((c>>5)&0x3F)*4,b=(c&0x1F)*8;return'rgb('+r+','+g+','+b+')';}";
    html += "function dimColor(r,g,b,f){return'rgb('+Math.floor(r/f)+','+Math.floor(g/f)+','+Math.floor(b/f)+')';}";
    html += "function initCanvas(){";
    html += "tftCanvas=document.getElementById('tftCanvas');";
    html += "if(!tftCanvas)return;";
    html += "tftCtx=tftCanvas.getContext('2d');";
    html += "tftCanvas.width=32*ledSize;";
    html += "tftCanvas.height=16*ledSize+gapSize;";
    html += "tftCtx.fillStyle='#000';tftCtx.fillRect(0,0,tftCanvas.width,tftCanvas.height);";
    html += "}";
    html += "function drawLED(x,y,lit,style,ledColor,surroundColor){";
    html += "var gap=(y>=8)?gapSize:0;";
    html += "var sx=x*ledSize,sy=y*ledSize+gap;";
    html += "var onCol=rgb565ToHex(ledColor);";
    html += "var surCol=rgb565ToHex(surroundColor);";
    html += "if(style===0){";
    html += "tftCtx.fillStyle=lit?onCol:'#000';";
    html += "tftCtx.fillRect(sx,sy,ledSize,ledSize);";
    html += "}else{";
    html += "tftCtx.fillStyle='#000';tftCtx.fillRect(sx,sy,ledSize,ledSize);";
    html += "if(lit){";
    html += "tftCtx.fillStyle=surCol;";
    html += "tftCtx.beginPath();tftCtx.arc(sx+ledSize/2,sy+ledSize/2,ledSize/2-1,0,Math.PI*2);tftCtx.fill();";
    html += "tftCtx.fillStyle=onCol;";
    html += "tftCtx.beginPath();tftCtx.arc(sx+ledSize/2,sy+ledSize/2,ledSize/2-2,0,Math.PI*2);tftCtx.fill();";
    html += "}else{";
    html += "tftCtx.fillStyle='#180000';";
    html += "tftCtx.beginPath();tftCtx.arc(sx+ledSize/2,sy+ledSize/2,ledSize/2-2,0,Math.PI*2);tftCtx.fill();";
    html += "}}}";
    html += "function updateDisplay(){";
    html += "fetch('/api/display')";
    html += ".then(function(r){return r.json();})";
    html += ".then(function(d){";
    html += "if(!tftCtx)initCanvas();";
    html += "if(!tftCtx)return;";
    html += "var buf=d.buffer,w=d.width,style=d.style,ledCol=d.ledColor,surCol=d.surroundColor;";
    html += "for(var row=0;row<2;row++){";
    html += "for(var x=0;x<32;x++){";
    html += "var byteVal=buf[x+row*32];";
    html += "for(var bit=0;bit<8;bit++){";
    html += "var y=row*8+bit;";
    html += "var lit=(byteVal&(1<<bit))!==0;";
    html += "drawLED(x,y,lit,style,ledCol,surCol);";
    html += "}}}})";
    html += ".catch(function(e){console.log('Display update failed:',e);});";
    html += "}";
    html += "setInterval(updateDisplay,500);";
    html += "setTimeout(function(){initCanvas();updateDisplay();},200);";
    html += "</script>";
    html += "</head><body>";
    html += "<div class='header'><h1>TFT LED Matrix Clock</h1></div>";

    html += "<div class='time-display'>";
    html += "<h2>Current Time & Environment</h2>";
    html += "<div class='clock' id='clock'>" + String(hours24) + ":" + String(minutes < 10 ? "0" : "") + String(minutes) + ":" + String(seconds < 10 ? "0" : "") + String(seconds) + "</div>";
    html += "<div class='date' id='date'>" + String(day < 10 ? "0" : "") + String(day) + "/" + String(month < 10 ? "0" : "") + String(month) + "/" + String(year) + "</div>";
    html += "</div>";

    // TFT Display Mirror Section
    html += "<div class='tft-mirror'>";
    html += "<h2>📺 TFT Display Mirror</h2>";
    html += "<div class='canvas-container'>";
    html += "<canvas id='tftCanvas'></canvas>";
    html += "</div>";
    html += "<p class='tft-label'>Live display - Updates every 500ms | 32×16 LED Matrix</p>";
    html += "</div>";

    if (sensorAvailable) {
      int tempDisplay = useFahrenheit ? (temperature * 9 / 5 + 32) : temperature;

      // Determine temperature icon based on temperature (Celsius for logic)
      String tempIcon = "🌡️";
      String tempColor = "#FFA500";
      if (temperature >= 30) {
        tempIcon = "🔥";
        tempColor = "#FF4444";
      } else if (temperature >= 25) {
        tempIcon = "☀️";
        tempColor = "#FFB347";
      } else if (temperature >= 20) {
        tempIcon = "🌤️";
        tempColor = "#FFD700";
      } else if (temperature >= 15) {
        tempIcon = "⛅";
        tempColor = "#87CEEB";
      } else if (temperature >= 10) {
        tempIcon = "☁️";
        tempColor = "#B0C4DE";
      } else if (temperature >= 5) {
        tempIcon = "🌧️";
        tempColor = "#4682B4";
      } else {
        tempIcon = "❄️";
        tempColor = "#00CED1";
      }

      // Determine humidity icon and color
      String humidityIcon = "💧";
      String humidityColor = "#4A90E2";
      if (humidity >= 70) {
        humidityIcon = "💦";
        humidityColor = "#1E90FF";
      } else if (humidity <= 30) {
        humidityIcon = "🏜️";
        humidityColor = "#DEB887";
      }

      html += "<div class='environment'>";
      html += "<div class='env-grid'>";

      // Temperature
      html += "<div class='env-item'>";
      html += "<span class='env-icon'>" + tempIcon + "</span>";
      html += "<div class='env-value' style='color:" + tempColor + ";text-shadow:0 0 20px " + tempColor + "44;'>" + String(tempDisplay) + (useFahrenheit ? "°F" : "°C") + "</div>";
      html += "<div class='env-label'>Temperature</div>";
      html += "</div>";

      // Humidity
      html += "<div class='env-item'>";
      html += "<span class='env-icon'>" + humidityIcon + "</span>";
      html += "<div class='env-value' style='color:" + humidityColor + ";text-shadow:0 0 20px " + humidityColor + "44;'>" + String(humidity) + "%</div>";
      html += "<div class='env-label'>Humidity</div>";
      html += "</div>";

      // Pressure
      html += "<div class='env-item'>";
      html += "<span class='env-icon'>🌍</span>";
      html += "<div class='env-value' style='color:#9370DB;text-shadow:0 0 20px #9370DB44;'>" + String(pressure) + "</div>";
      html += "<div class='env-label'>Pressure (hPa)</div>";
      html += "</div>";

      html += "</div></div>";
    }
    
    html += "<div class='card'><h2>Settings</h2>";
    html += "<button onclick=\"location.href='/temperature?mode=toggle'\">Toggle °C/°F</button>";
    html += "</div>";
    
    html += "<div class='card'><h2>Display Style</h2>";
    html += "<p>Current Style: " + String(displayStyle == 0 ? "Default (Blocks)" : "Realistic (LEDs)") + "</p>";
    html += "<button onclick=\"location.href='/style?mode=toggle'\">Toggle Style</button><br><br>";
    
    html += "<p>LED Color:</p>";
    html += "<select id='ledcolor' onchange=\"location.href='/style?ledcolor='+this.value\">";
    html += "<option value='0'" + String(ledOnColor == COLOR_RED ? " selected" : "") + ">Red</option>";
    html += "<option value='1'" + String(ledOnColor == COLOR_GREEN ? " selected" : "") + ">Green</option>";
    html += "<option value='2'" + String(ledOnColor == COLOR_BLUE ? " selected" : "") + ">Blue</option>";
    html += "<option value='3'" + String(ledOnColor == COLOR_YELLOW ? " selected" : "") + ">Yellow</option>";
    html += "<option value='4'" + String(ledOnColor == COLOR_CYAN ? " selected" : "") + ">Cyan</option>";
    html += "<option value='5'" + String(ledOnColor == COLOR_MAGENTA ? " selected" : "") + ">Magenta</option>";
    html += "<option value='6'" + String(ledOnColor == COLOR_WHITE ? " selected" : "") + ">White</option>";
    html += "<option value='7'" + String(ledOnColor == COLOR_ORANGE ? " selected" : "") + ">Orange</option>";
    html += "</select><br><br>";
    
    html += "<p>Surround Color:</p>";
    html += "<select id='surroundcolor' onchange=\"location.href='/style?surroundcolor='+this.value\">";
    html += "<option value='0'" + String(ledSurroundColor == COLOR_WHITE ? " selected" : "") + ">White</option>";
    html += "<option value='1'" + String(ledSurroundColor == COLOR_LIGHT_GRAY ? " selected" : "") + ">Light Gray</option>";
    html += "<option value='2'" + String(ledSurroundColor == COLOR_DARK_GRAY ? " selected" : "") + ">Dark Gray</option>";
    html += "<option value='3'" + String(ledSurroundColor == COLOR_RED ? " selected" : "") + ">Red</option>";
    html += "<option value='4'" + String(ledSurroundColor == COLOR_GREEN ? " selected" : "") + ">Green</option>";
    html += "<option value='5'" + String(ledSurroundColor == COLOR_BLUE ? " selected" : "") + ">Blue</option>";
    html += "<option value='6'" + String(ledSurroundColor == COLOR_YELLOW ? " selected" : "") + ">Yellow</option>";
    html += "<option value='7'" + String(ledSurroundColor == ledOnColor ? " selected" : "") + ">Match LED Color</option>";
    html += "</select>";
    html += "</div>";
    
    html += "<div class='card'><h2>Timezone & Time Format</h2>";
    html += "<p>Current Timezone: " + String(timezones[currentTimezone].name) + "</p>";
    html += "<select id='tz' onchange=\"location.href='/timezone?tz='+this.value\">";
    for (int i = 0; i < numTimezones; i++) {
      html += "<option value='" + String(i) + "'" + (i == currentTimezone ? " selected" : "") + ">";
      html += timezones[i].name;
      html += "</option>";
    }
    html += "</select><br><br>";
    
    html += "<p>Time Format: " + String(use24HourFormat ? "24-Hour" : "12-Hour") + "</p>";
    html += "<button onclick=\"location.href='/timeformat?mode=toggle'\">Toggle 12/24 Hour</button>";
    if (use24HourFormat) {
      html += "<p style='color:#666;font-size:12px;margin-top:10px;'>⚠️ Note: In Time+Temp mode, seconds not displayed when hours ≥ 10 due to space constraints</p>";
    }
    html += "</div>";
    
    html += "<div class='card'><h2>System</h2>";
    html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
    html += "<p>Uptime: " + String(millis() / 1000) + "s</p>";
    html += "<button onclick=\"if(confirm('Reset WiFi?'))location.href='/reset'\">Reset WiFi</button>";
    html += "</div>";

    // About footer
    html += "<div class='footer'>";
    html += "<p>ESP8266 TFT LED Retro Clock v" + String(VERSION) + "</p>";
    html += "<p>Created by <a href='https://bsky.app/profile/anthonyclarke.bsky.social' target='_blank'>Anthony Clarke</a></p>";
    html += "<p><a href='https://github.com/anthonyjclarke/ESP8266_TFT_RetroClock' target='_blank'>GitHub Repository</a></p>";
    html += "</div>";

    html += "</body></html>";
    server.send(200, "text/html", html);
  });
  
  // API endpoints
  server.on("/api/time", []() {
    String json = "{\"hours\":" + String(hours24) + ",\"minutes\":" + String(minutes) +
                  ",\"seconds\":" + String(seconds) + ",\"day\":" + String(day) +
                  ",\"month\":" + String(month) + ",\"year\":" + String(year) +
                  ",\"use24hour\":" + String(use24HourFormat ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });
  
  // Display buffer API endpoint - returns 64-byte screen buffer and display settings
  // This enables real-time TFT display mirroring on the web page with minimal overhead
  server.on("/api/display", []() {
    String json = "{\"buffer\":[";
    for (int i = 0; i < LINE_WIDTH * DISPLAY_ROWS; i++) {
      json += String(scr[i]);
      if (i < LINE_WIDTH * DISPLAY_ROWS - 1) json += ",";
    }
    json += "],\"style\":" + String(displayStyle);
    json += ",\"ledColor\":" + String(ledOnColor);
    json += ",\"surroundColor\":" + String(ledSurroundColor);
    json += ",\"width\":" + String(TOTAL_WIDTH);
    json += ",\"height\":" + String(TOTAL_HEIGHT);
    json += "}";
    server.send(200, "application/json", json);
  });
  
  server.on("/api/status", []() {
    int tempDisplay = useFahrenheit ? (temperature * 9 / 5 + 32) : temperature;
    String json = "{\"sensor_available\":" + String(sensorAvailable ? "true" : "false") +
                  ",\"temperature\":" + String(tempDisplay) +
                  ",\"humidity\":" + String(humidity) +
                  ",\"pressure\":" + String(pressure) +
                  ",\"temp_unit\":\"" + String(useFahrenheit ? "Fahrenheit" : "Celsius") + "\"}";
    server.send(200, "application/json", json);
  });
  
  // Temperature unit toggle endpoint
  server.on("/temperature", []() {
    if (server.hasArg("mode")) {
      useFahrenheit = !useFahrenheit;
      DEBUG(Serial.printf("Temperature unit: %s\n", useFahrenheit ? "Fahrenheit" : "Celsius"));

      // Force immediate display update
      switch (currentMode) {
        case 0: displayTimeAndTemp(); break;
        case 1: displayTimeLarge(); break;
        case 2: displayTimeAndDate(); break;
      }
      refreshAll();
    }
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  
  // Timezone configuration endpoint
  server.on("/timezone", []() {
    if (server.hasArg("tz")) {
      int newTimezone = server.arg("tz").toInt();
      if (newTimezone >= 0 && newTimezone < numTimezones) {
        currentTimezone = newTimezone;
        DEBUG(Serial.printf("Timezone changed to: %s\n", timezones[currentTimezone].name));
        syncNTP();
      }
    }
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  
  // Time format (12/24 hour) toggle endpoint
  server.on("/timeformat", []() {
    if (server.hasArg("mode")) {
      use24HourFormat = !use24HourFormat;
      DEBUG(Serial.printf("Time format changed to: %s\n", use24HourFormat ? "24-Hour" : "12-Hour"));

      // Force immediate display update
      forceFullRedraw = true;
      switch (currentMode) {
        case 0: displayTimeAndTemp(); break;
        case 1: displayTimeLarge(); break;
        case 2: displayTimeAndDate(); break;
      }
      refreshAll();
    }
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  
  // Display style configuration endpoint
  server.on("/style", []() {
    bool changed = false;
    
    // Toggle display style
    if (server.hasArg("mode") && server.arg("mode") == "toggle") {
      displayStyle = (displayStyle == 0) ? 1 : 0;
      changed = true;
      DEBUG(Serial.printf("Display style toggled to: %d (%s)\n", 
                          displayStyle, displayStyle == 0 ? "Default" : "Realistic"));
    }
    
    // Set LED color
    if (server.hasArg("ledcolor")) {
      int colorIdx = server.arg("ledcolor").toInt();
      switch(colorIdx) {
        case 0: ledOnColor = COLOR_RED; break;
        case 1: ledOnColor = COLOR_GREEN; break;
        case 2: ledOnColor = COLOR_BLUE; break;
        case 3: ledOnColor = COLOR_YELLOW; break;
        case 4: ledOnColor = COLOR_CYAN; break;
        case 5: ledOnColor = COLOR_MAGENTA; break;
        case 6: ledOnColor = COLOR_WHITE; break;
        case 7: ledOnColor = COLOR_ORANGE; break;
        default: ledOnColor = COLOR_RED;
      }
      // Update the off color to be a dim version
      ledOffColor = ledOnColor >> 3;  // Dim by dividing RGB components
      
      // If surround is in "Match LED Color" mode, update it too
      if (surroundMatchesLED) {
        ledSurroundColor = ledOnColor;
      }
      
      changed = true;
      DEBUG(Serial.printf("LED color changed to index: %d\n", colorIdx));
    }
    
    // Set surround color
    if (server.hasArg("surroundcolor")) {
      int colorIdx = server.arg("surroundcolor").toInt();
      switch(colorIdx) {
        case 0: 
          ledSurroundColor = COLOR_WHITE; 
          surroundMatchesLED = false;
          break;
        case 1: 
          ledSurroundColor = COLOR_LIGHT_GRAY; 
          surroundMatchesLED = false;
          break;
        case 2: 
          ledSurroundColor = COLOR_DARK_GRAY; 
          surroundMatchesLED = false;
          break;
        case 3: 
          ledSurroundColor = COLOR_RED; 
          surroundMatchesLED = false;
          break;
        case 4: 
          ledSurroundColor = COLOR_GREEN; 
          surroundMatchesLED = false;
          break;
        case 5: 
          ledSurroundColor = COLOR_BLUE; 
          surroundMatchesLED = false;
          break;
        case 6: 
          ledSurroundColor = COLOR_YELLOW; 
          surroundMatchesLED = false;
          break;
        case 7: 
          // Match LED color mode
          ledSurroundColor = ledOnColor;
          surroundMatchesLED = true;  // Track that we're in match mode
          break;
        default: 
          ledSurroundColor = COLOR_WHITE;
          surroundMatchesLED = false;
      }
      changed = true;
      DEBUG(Serial.printf("Surround color changed to index: %d, match mode: %s\n", 
                          colorIdx, surroundMatchesLED ? "ON" : "OFF"));
    }
    
    // Force a complete redraw if anything changed
    if (changed) {
      // Clear the entire screen to black
      tft.fillScreen(BG_COLOR);
      
      // Set the global flag to force FAST_REFRESH to ignore its cache
      forceFullRedraw = true;
      
      // Immediately trigger display update with new colors
      // This ensures instant visual feedback instead of waiting for next second
      switch (currentMode) {
        case 0: displayTimeAndTemp(); break;
        case 1: displayTimeLarge(); break;
        case 2: displayTimeAndDate(); break;
      }
      refreshAll();  // Draw immediately with new colors

      DEBUG(Serial.println("Style changed - immediate redraw complete"));
    }
    
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  
  // Reset WiFi
  server.on("/reset", []() {
    server.send(200, "text/html", 
      "<html><body><h1>WiFi Reset</h1><p>WiFi settings cleared. Device will restart...</p></body></html>");
    delay(1000);
    wifiManager.resetSettings();
    ESP.restart();
  });
  
  server.begin();
  DEBUG(Serial.println("Web server started"));
}

// ======================== FORWARD DECLARATIONS ========================
void configModeCallback(WiFiManager* myWiFiManager);

// ======================== SETUP ========================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  DEBUG(Serial.println("\n\n╔════════════════════════════════════════╗"));
  // Calculate padding to keep box width at 40 chars (30 + version length + padding = 40)
  int versionLen = strlen(VERSION);
  int padding = 40 - 30 - versionLen;  // 30 = length of "   ESP8266 TFT Matrix Clock v"
  DEBUG(Serial.printf("║   ESP8266 TFT Matrix Clock v%s%*s║\n", VERSION, padding, ""));
  DEBUG(Serial.println("║   TFT Display Edition                  ║"));
  DEBUG(Serial.println("╚════════════════════════════════════════╝\n"));
  
  // Initialize TFT display
  initTFT();

  showMessage("INIT");

  // Test sensor
  sensorAvailable = testSensor();
  if (sensorAvailable) {
    updateSensorData();
  }
  
  // WiFi setup
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(180);
  
  if (!wifiManager.autoConnect("TFT_Clock_Setup")) {
    DEBUG(Serial.println("Failed to connect, restarting..."));
    delay(3000);
    ESP.restart();
  }
  
  DEBUG(Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str()));
  showMessage("WIFI OK");
  delay(1000);
  
  // Sync time
  syncNTP();
  showMessage("TIME OK");
  delay(1000);
  
  // Start web server
  setupWebServer();
  showMessage("READY");
  delay(1000);

  // Initialize display with current time
  clearScreen();
  tft.fillScreen(BG_COLOR);
  updateTime();

  lastNTPSync = millis();
  lastSensorUpdate = millis();
  lastStatusPrint = millis();
  lastModeSwitch = millis();
}

// ======================== MAIN LOOP ========================

void loop() {
  server.handleClient();
  
  unsigned long now = millis();
  
  // Update time
  updateTime();

  // Update sensor data
  if (sensorAvailable && now - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
    updateSensorData();
    lastSensorUpdate = now;
  }
  
  // Periodic NTP sync
  if (now - lastNTPSync >= NTP_SYNC_INTERVAL) {
    syncNTP();
    lastNTPSync = now;
  }
  
  // Print status
  if (now - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
    DEBUG(Serial.printf("Time: %02d:%02d | Date: %02d/%02d/%04d | Temp: %d°C | Hum: %d%% | Pressure: %d hPa\n",
                        hours24, minutes, day, month, year, temperature, humidity, pressure));
    lastStatusPrint = now;
  }
  
  delay(100);
}

// ======================== HELPER FUNCTIONS ========================

void configModeCallback(WiFiManager* myWiFiManager) {
  DEBUG(Serial.println("\n=== WiFi Config Mode ==="));
  DEBUG(Serial.println("Connect to AP: TFT_Clock_Setup"));
  DEBUG(Serial.print("Config portal IP: "));
  DEBUG(Serial.println(WiFi.softAPIP()));
  
  showMessage("SETUP AP");
}
