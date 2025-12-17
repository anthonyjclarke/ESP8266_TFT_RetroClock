#include "Arduino.h"

/*
 * ESP8266 TFT Matrix Clock - TFT Edition
 * Author: Refactored from LED Matrix version by Anthony Clarke
 * Display: 1.8/2.4/2.8 Inch SPI TFT LCD (ILI9341/ST7789)
 * 
 * This version simulates the LED matrix appearance on a TFT display
 * All functionality remains identical to the original LED matrix version
 * 
 * ======================== FEATURES ========================
 * - Simulates 4x MAX7219 x 2 Rows LED matrix appearance on TFT display
 * - WiFiManager for easy WiFi setup (no hardcoded credentials)
 * - BME280 I2C temperature/pressure/humidity sensor
 * - Automatic NTP time synchronization with DST support
 * - Web interface for status and configuration
 * - Display always on with backlight permanently enabled
 * - Multiple timezone support with POSIX TZ strings
 
 * =========================== TODO ==========================
 * - Add 12/24 Switch on Web interface
 * - Move TZ selection into header file for easier editing
 * - Get weather from online API and display on matrix and webpage
 * - Implement web interface for full configuration
 * - Add OTA firmware update capability
 * - Tidy up web interface and combine all configuration into single page
 * 
 *
 * ======================== CHANGELOG ========================
 * v1.0 17th December 2025
 *  - Initial TFT version based on LED matrix code
 * 
 * 
*/

// ======================== LIBRARIES ========================
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <time.h>
#include <TZ.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>  // For ILI9341 displays
// #include <Adafruit_ST7789.h>  // Uncomment for ST7789 displays

// ======================== DISPLAY SELECTION ========================
// Uncomment ONE of the following display types:
#define USE_ILI9341
// #define USE_ST7789

// ======================== PIN DEFINITIONS ========================
// TFT Display SPI Pins
#define LED_PIN   D8    // TFT Backlight (if applicable)
#define TFT_CS    D1    // TFT Chip Select
#define TFT_DC    D2    // TFT Data/Command
//#define TFT_RST   D4    // TFT Reset (or connect to ESP reset)
//#define MOSI      D7    // MOSI (hardware SPI)
//#define SCK       D5    // SCK (hardware SPI)

// Sensor and Control Pins
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

// Calculate display dimensions
// When LED_SPACING = 0, formula simplifies to: LED_SIZE * count
#define DISPLAY_WIDTH     (LED_SIZE * TOTAL_WIDTH)
#define DISPLAY_HEIGHT    (LED_SIZE * TOTAL_HEIGHT)

// ======================== TIMING CONFIGURATION ========================
#define SENSOR_UPDATE_INTERVAL       60000  // Update sensor every 60s
#define NTP_SYNC_INTERVAL            3600000 // Sync NTP every hour
#define STATUS_PRINT_INTERVAL        10000  // Print status every 10s

// ======================== DEBUG CONFIGURATION ========================
#define DEBUG_ENABLED 1

// ======================== DISPLAY OPTIMIZATION ========================
#define FAST_REFRESH 1      // Set to 1 to only redraw changed pixels (much faster)

#if DEBUG_ENABLED
  #define DEBUG(x) x
#else
  #define DEBUG(x)
#endif

// ======================== DISPLAY OBJECT ========================
#ifdef USE_ILI9341
  Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC /*, TFT_RST*/);
#elif defined(USE_ST7789)
  Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
#endif

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

// ======================== TIME VARIABLES ========================
int hours = 0, minutes = 0, seconds = 0;
int hours24 = 0;  // 24-hour format
int day = 1, month = 1, year = 2025;
int lastSecond = -1;

// ======================== SENSOR VARIABLES ========================
bool sensorAvailable = false;
int temperature = 0;
int humidity = 0;
int pressure = 0;
bool useFahrenheit = false;

// ======================== UPDATE TIMERS ========================
unsigned long lastSensorUpdate = 0;
unsigned long lastNTPSync = 0;
unsigned long lastStatusPrint = 0;

// ======================== DISPLAY MODES ========================
int currentMode = 0; // 0=Time+Temp, 1=Time Large, 2=Time+Date
unsigned long lastModeSwitch = 0;
#define MODE_SWITCH_INTERVAL 5000

// ======================== TIMEZONE CONFIGURATION ========================
struct TimezoneInfo {
  const char* name;
  const char* tzString;
};

// Expanded timezone array with 88 global timezones
TimezoneInfo timezones[] = {
  {"Sydney, Australia", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"Melbourne, Australia", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"Brisbane, Australia", "AEST-10"},
  {"Adelaide, Australia", "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
  {"Perth, Australia", "AWST-8"},
  {"Darwin, Australia", "ACST-9:30"},
  {"Hobart, Australia", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"New York, USA", "EST5EDT,M3.2.0,M11.1.0"},
  {"Los Angeles, USA", "PST8PDT,M3.2.0,M11.1.0"},
  {"Chicago, USA", "CST6CDT,M3.2.0,M11.1.0"},
  {"Denver, USA", "MST7MDT,M3.2.0,M11.1.0"},
  {"Phoenix, USA", "MST7"},
  {"Anchorage, USA", "AKST9AKDT,M3.2.0,M11.1.0"},
  {"Honolulu, USA", "HST10"},
  {"London, UK", "GMT0BST,M3.5.0/1,M10.5.0"},
  {"Paris, France", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Berlin, Germany", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Rome, Italy", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Madrid, Spain", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Amsterdam, Netherlands", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Brussels, Belgium", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Vienna, Austria", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Zurich, Switzerland", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Stockholm, Sweden", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Oslo, Norway", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Copenhagen, Denmark", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Helsinki, Finland", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Athens, Greece", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Moscow, Russia", "MSK-3"},
  {"Dubai, UAE", "GST-4"},
  {"Mumbai, India", "IST-5:30"},
  {"Bangkok, Thailand", "ICT-7"},
  {"Singapore", "SGT-8"},
  {"Hong Kong", "HKT-8"},
  {"Shanghai, China", "CST-8"},
  {"Tokyo, Japan", "JST-9"},
  {"Seoul, South Korea", "KST-9"},
  {"Auckland, New Zealand", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
  {"Wellington, New Zealand", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
  {"Fiji", "FJT-12FJST,M11.1.0,M1.3.0/3"},
  {"Toronto, Canada", "EST5EDT,M3.2.0,M11.1.0"},
  {"Vancouver, Canada", "PST8PDT,M3.2.0,M11.1.0"},
  {"Montreal, Canada", "EST5EDT,M3.2.0,M11.1.0"},
  {"Mexico City, Mexico", "CST6CDT,M4.1.0,M10.5.0"},
  {"Sao Paulo, Brazil", "BRT3BRST,M10.3.0/0,M2.3.0/0"},
  {"Buenos Aires, Argentina", "ART3"},
  {"Santiago, Chile", "CLT4CLST,M8.2.6/24,M5.2.6/24"},
  {"Lima, Peru", "PET5"},
  {"Bogota, Colombia", "COT5"},
  {"Caracas, Venezuela", "VET4:30"},
  {"Johannesburg, South Africa", "SAST-2"},
  {"Cairo, Egypt", "EET-2"},
  {"Lagos, Nigeria", "WAT-1"},
  {"Nairobi, Kenya", "EAT-3"},
  {"Tel Aviv, Israel", "IST-2IDT,M3.4.4/26,M10.5.0"},
  {"Istanbul, Turkey", "TRT-3"},
  {"Riyadh, Saudi Arabia", "AST-3"},
  {"Karachi, Pakistan", "PKT-5"},
  {"Dhaka, Bangladesh", "BST-6"},
  {"Yangon, Myanmar", "MMT-6:30"},
  {"Jakarta, Indonesia", "WIB-7"},
  {"Manila, Philippines", "PHT-8"},
  {"Taipei, Taiwan", "CST-8"},
  {"Kuala Lumpur, Malaysia", "MYT-8"},
  {"Ho Chi Minh, Vietnam", "ICT-7"},
  {"Kabul, Afghanistan", "AFT-4:30"},
  {"Tehran, Iran", "IRST-3:30IRDT,J79/24,J263/24"},
  {"Reykjavik, Iceland", "GMT0"},
  {"Lisbon, Portugal", "WET0WEST,M3.5.0/1,M10.5.0"},
  {"Dublin, Ireland", "IST-1GMT0,M10.5.0,M3.5.0/1"},
  {"Prague, Czech Republic", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Warsaw, Poland", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Budapest, Hungary", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Bucharest, Romania", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Sofia, Bulgaria", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Kiev, Ukraine", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Minsk, Belarus", "MSK-3"},
  {"Yerevan, Armenia", "AMT-4"},
  {"Tbilisi, Georgia", "GET-4"},
  {"Baku, Azerbaijan", "AZT-4"},
  {"Tashkent, Uzbekistan", "UZT-5"},
  {"Almaty, Kazakhstan", "ALMT-6"},
  {"Bishkek, Kyrgyzstan", "KGT-6"},
  {"Colombo, Sri Lanka", "IST-5:30"},
  {"Kathmandu, Nepal", "NPT-5:45"},
  {"Thimphu, Bhutan", "BTT-6"},
  {"Ulaanbaatar, Mongolia", "ULAT-8"},
  {"Port Moresby, Papua New Guinea", "PGT-10"},
  {"Noumea, New Caledonia", "NCT-11"}
};

int currentTimezone = 0;
const int numTimezones = sizeof(timezones) / sizeof(timezones[0]);

// ======================== TFT DISPLAY FUNCTIONS ========================

void initTFT() {
  DEBUG(Serial.println("Initializing TFT Display..."));
  
  // Add small delay before TFT initialization
  delay(100);
  
  #ifdef USE_ILI9341
    tft.begin();
    tft.setRotation(3);  // Try rotation 3 (landscape, flipped) instead of 1
    DEBUG(Serial.printf("ILI9341 initialized, rotation set to 3\n"));
  #elif defined(USE_ST7789)
    tft.init(240, 320);  // Initialize for specific resolution
    tft.setRotation(3);  // Try rotation 3 (landscape, flipped) instead of 1
    DEBUG(Serial.printf("ST7789 initialized, rotation set to 3\n"));
  #endif
  
  // Small delay after initialization
  delay(100);
  
  // Check actual dimensions
  DEBUG(Serial.printf("TFT reports dimensions: %d x %d\n", tft.width(), tft.height()));
  
  tft.fillScreen(BG_COLOR);
  pinMode(LED_PIN, D8);
  digitalWrite(LED_PIN, HIGH);
  
  // Calculate display dimensions
  int displayWidth = tft.width();
  int displayHeight = tft.height();
  int offsetX = max(0, (displayWidth - DISPLAY_WIDTH) / 2);
  int offsetY = max(0, (displayHeight - DISPLAY_HEIGHT) / 2);
  
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

void drawLEDPixel(int x, int y, bool lit) {
  // Bounds checking
  if (x < 0 || x >= TOTAL_WIDTH || y < 0 || y >= TOTAL_HEIGHT) {
    return;
  }
  
  // Calculate screen position with centering offset
  // Note: Recalculate each time to ensure we use correct DISPLAY_WIDTH/HEIGHT
  int offsetX = max(0, (tft.width() - DISPLAY_WIDTH) / 2);
  int offsetY = max(0, (tft.height() - DISPLAY_HEIGHT) / 2);
  
  int screenX = offsetX + x * LED_SIZE;
  int screenY = offsetY + y * LED_SIZE;
  
  uint16_t color;
  if (lit) {
    // Draw lit LEDs using the configured LED_COLOR
    color = LED_COLOR;
  } else {
    color = LED_OFF_COLOR;  // Dim "off" LED
  }
  
  // Use fillRect instead of fillRoundRect - MUCH faster (no anti-aliasing)
  tft.fillRect(screenX, screenY, LED_SIZE, LED_SIZE, color);
}

void refreshAll() {
  // The buffer is organized as scr[x + y * LINE_WIDTH] where each byte = 8 vertical pixels
  // We have 2 rows of matrices, so we need to handle 16 pixels vertically
  
  #if FAST_REFRESH
    // Static buffer to track previous state for change detection
    static byte lastScr[LINE_WIDTH * DISPLAY_ROWS] = {0};
    static bool firstRun = true;
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
  
  // Top row (yPos = 0): Time - using 12-hour format with seconds
  int x = (hours > 9) ? 0 : 2;
  sprintf(buf, "%d", hours);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x8rn) + 1;
  }
  
  if (showDots) {
    x += drawCharWithY(x, 0, ':', digits5x8rn) + 1;
  } else {
    x += 2;  // Space for colon
  }
  
  x += (hours >= 20) ? 0 : 1;
  sprintf(buf, "%02d", minutes);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x8rn) + 1;
  }
  
  sprintf(buf, "%02d", seconds);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits3x5) + 1;
  }
  
  // Bottom row (yPos = 1): Temperature and Humidity
  x = 1;
  if (sensorAvailable) {
    int displayTemp = useFahrenheit ? (temperature * 9 / 5 + 32) : temperature;
    char tempUnit = useFahrenheit ? 'F' : 'C';
    sprintf(buf, "T%d%c H%d%%", displayTemp, tempUnit, humidity);
  } else {
    sprintf(buf, "NO SENSOR");
  }
  
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 1, *p, font3x7) + 1;
  }
  
  // Shift bottom line slightly (as in original)
  for (int i = 0; i < LINE_WIDTH; i++) {
    scr[LINE_WIDTH + i] <<= 1;
  }
}

void displayTimeLarge() {
  clearScreen();
  
  char buf[32];
  bool showDots = (seconds % 2) == 0;
  
  // Large time display on top row using 16-pixel tall font
  int x = (hours > 9) ? 0 : 3;
  sprintf(buf, "%d", hours);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x16rn) + 1;
  }
  
  if (showDots) {
    x += drawCharWithY(x, 0, ':', digits5x16rn) + 1;
  } else {
    x += 2;
  }
  
  x += (hours >= 20) ? 0 : 1;
  sprintf(buf, "%02d", minutes);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x16rn) + 1;
  }
  
  sprintf(buf, "%02d", seconds);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, font3x7) + 1;
  }
}

void displayTimeAndDate() {
  clearScreen();
  
  char buf[32];
  bool showDots = (seconds % 2) == 0;
  
  // Top row: Time
  int x = 0;
  sprintf(buf, "%02d", hours24);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x8rn) + 1;
  }
  
  if (showDots) {
    x += drawCharWithY(x, 0, ':', digits5x8rn) + 1;
  } else {
    x += 2;
  }
  
  x += 1;
  sprintf(buf, "%02d", minutes);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 0, *p, digits5x8rn) + 1;
  }
  
  // Bottom row: Date
  x = 2;
  sprintf(buf, "%02d/%02d/%02d", day, month, year % 100);
  for (const char* p = buf; *p; p++) {
    x += drawCharWithY(x, 1, *p, font3x7) + 1;
  }
  
  // Shift bottom line slightly
  for (int i = 0; i < LINE_WIDTH; i++) {
    scr[LINE_WIDTH + i] <<= 1;
  }
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
  
  DEBUG(Serial.printf("Sensor: %d°C, %d%% RH, %d hPa\n", temperature, humidity, pressure));
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
    html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;}";
    html += ".card{background:white;padding:20px;margin:10px 0;border-radius:5px;box-shadow:0 2px 5px rgba(0,0,0,0.1);}";
    html += "h1{color:#333;text-align:center;}";
    html += "h2{color:#666;border-bottom:2px solid #4CAF50;padding-bottom:5px;}";
    html += "button{background:#4CAF50;color:white;border:none;padding:10px 20px;cursor:pointer;border-radius:3px;margin:5px;}";
    html += "button:hover{background:#45a049;}";
    html += "select{padding:5px;font-size:14px;}";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>🎨 TFT LED Matrix Clock</h1>";
    
    html += "<div class='card'><h2>Current Time</h2>";
    html += "<p>Time: " + String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes) + "</p>";
    html += "<p>Date: " + String(day) + "/" + String(month) + "/" + String(year) + "</p>";
    html += "</div>";
    
    if (sensorAvailable) {
      int tempDisplay = useFahrenheit ? (temperature * 9 / 5 + 32) : temperature;
      html += "<div class='card'><h2>Environment</h2>";
      html += "<p>Temperature: " + String(tempDisplay) + (useFahrenheit ? "°F" : "°C") + "</p>";
      html += "<p>Humidity: " + String(humidity) + "%</p>";
      html += "<p>Pressure: " + String(pressure) + " hPa</p>";
      html += "</div>";
    }
    
    html += "<div class='card'><h2>Settings</h2>";
    html += "<p>Temperature Unit: " + String(useFahrenheit ? "Fahrenheit" : "Celsius") + "</p>";
    html += "<button onclick=\"location.href='/temperature?mode=toggle'\">Toggle °C/°F</button>";
    html += "</div>";
    
    html += "<div class='card'><h2>Timezone</h2>";
    html += "<p>Current: " + String(timezones[currentTimezone].name) + "</p>";
    html += "<select id='tz' onchange=\"location.href='/timezone?tz='+this.value\">";
    for (int i = 0; i < numTimezones; i++) {
      html += "<option value='" + String(i) + "'" + (i == currentTimezone ? " selected" : "") + ">";
      html += timezones[i].name;
      html += "</option>";
    }
    html += "</select>";
    html += "</div>";
    
    html += "<div class='card'><h2>System</h2>";
    html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
    html += "<p>Uptime: " + String(millis() / 1000) + "s</p>";
    html += "<button onclick=\"if(confirm('Reset WiFi?'))location.href='/reset'\">Reset WiFi</button>";
    html += "</div>";
    
    html += "</body></html>";
    server.send(200, "text/html", html);
  });
  
  // API endpoints
  server.on("/api/time", []() {
    String json = "{\"hours\":" + String(hours24) + ",\"minutes\":" + String(minutes) + 
                  ",\"seconds\":" + String(seconds) + ",\"day\":" + String(day) + 
                  ",\"month\":" + String(month) + ",\"year\":" + String(year) + "}";
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
  DEBUG(Serial.println("║   ESP8266 TFT Matrix Clock             ║"));
  DEBUG(Serial.println("║   TFT Display Edition                  ║"));
  DEBUG(Serial.println("╚════════════════════════════════════════╝\n"));
  
  // Initialize TFT display
  initTFT();
  
  // Setup backlight pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // Turn backlight ON
  
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
    DEBUG(Serial.printf("Time: %02d:%02d | Temp: %d°C | Hum: %d%%\n",
                        hours24, minutes, temperature, humidity));
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
