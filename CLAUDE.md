# Project: ESP8266 TFT LED Retro Clock

## Overview
A feature-rich retro clock that simulates the appearance of MAX7219 LED matrix modules on an ILI9341/ST7789 TFT display. The clock provides realistic circular LED rendering with customizable colors, BME280 environmental monitoring, NTP time synchronization with 88 global timezones, and a modern responsive web interface with live updates. Three auto-cycling display modes show time with temperature, large time display, or time with date - all with seconds display.

## Hardware
- **MCU:** ESP8266 D1 Mini Pro (80KB RAM minimum)
- **Display:** 1.8"-2.8" ILI9341 or ST7789 SPI TFT LCD (240x320 pixels, 40MHz SPI)
- **Sensor:** Adafruit BME280 I2C (Temperature/Humidity/Pressure) - Optional but recommended
- **Power:** 5V USB (~250mA active)
- **Network:** 2.4GHz WiFi only

## Build Environment
- **Framework:** Arduino
- **Platform:** espressif8266 (PlatformIO)
- **Monitor/Upload Speed:** 115200 baud
- **Key Libraries:**
  - TFT_eSPI@^2.5.43 (hardware-optimized display driver, 40MHz SPI)
  - WiFiManager@^2.0.17 (captive portal WiFi setup)
  - Adafruit BME280 Library@^2.3.0
  - Adafruit Unified Sensor@^1.1.14
  - ESP8266WiFi, ESP8266WebServer (built-in)
  - Wire, time.h, TZ.h (built-in)

## Project Structure
```
ESP8266_TFT_LED_RetroClock/
├── platformio.ini              # PlatformIO build configuration
├── README.md                   # Comprehensive user documentation
├── CLAUDE.md                   # This file - project context for Claude
├── include/
│   ├── User_Setup.h           # TFT_eSPI display configuration (ILI9341, pins, SPI speed)
│   ├── fonts.h                # Bitmap font definitions in PROGMEM (5 fonts)
│   └── timezones.h            # 88 global timezone POSIX TZ strings
├── src/
│   └── main_tft.cpp           # Main application (~1400 lines)
│       ├── Display rendering (LED matrix simulation)
│       ├── Font rendering (columnar bitmap fonts)
│       ├── Web server & REST API
│       ├── NTP time sync & timezone handling
│       ├── BME280 sensor management
│       └── Three display modes with auto-cycling
└── .pio/                       # PlatformIO build artifacts & dependencies
```

## Pin Mapping

### TFT Display (SPI)
| Function | GPIO | D1 Mini | Notes |
|----------|------|---------|-------|
| TFT_CS   | 5    | D1      | Chip Select |
| TFT_DC   | 4    | D2      | Data/Command |
| TFT_RST  | -1   | RST/3.3V| Connected to ESP RST or 3.3V (no software reset) |
| TFT_MOSI | 13   | D7      | Hardware SPI - Data Out |
| TFT_SCK  | 14   | D5      | Hardware SPI - Clock |
| LED      | 15   | D8      | Backlight control (HIGH=ON) |

### BME280 Sensor (I2C)
| Function | GPIO | D1 Mini | Notes |
|----------|------|---------|-------|
| SDA      | 2    | D4      | I2C Data |
| SCL      | 0    | D3      | I2C Clock |

**Important:** TFT pins (D1, D2, D5, D7, D8) and BME280 pins (D3, D4) are completely separate - no conflicts.

## Configuration

### Build-Time Configuration (src/main_tft.cpp)
```cpp
// Display Configuration
#define LED_SIZE 10                    // Each LED = 10x10 pixels
#define DEFAULT_DISPLAY_STYLE 1        // 0=Default blocks, 1=Realistic LEDs
#define SENSOR_UPDATE_INTERVAL 60000   // Sensor polling (60s)
#define NTP_SYNC_INTERVAL 3600000      // NTP sync (1 hour)
#define MODE_SWITCH_INTERVAL 5000      // Display mode auto-cycle (5s)

// Optimization Flags
#define BRIGHTNESS_BOOST 1             // Maximum brightness (ignores brightness level)
#define FAST_REFRESH 1                 // Only redraw changed pixels (6-8x faster)
#define DEBUG_ENABLED 1                // Serial debug output
```

### Display Configuration (include/User_Setup.h)
```cpp
#define ILI9341_DRIVER              // Display type (ILI9341 or ST7789)
#define TFT_CS   PIN_D1             // See pin mapping above
#define TFT_DC   PIN_D2
#define TFT_RST  -1
#define SPI_FREQUENCY  40000000     // 40MHz SPI bus speed
```

### Runtime Configuration (Web Interface)
- **Timezone:** 90 global options with POSIX TZ strings (automatic DST)
- **Time Format:** 12-hour or 24-hour toggle
- **Temperature Unit:** Celsius or Fahrenheit toggle
- **Display Style:** Default (blocks) or Realistic (circular LEDs)
- **LED Color:** 8 options (Red, Green, Blue, Yellow, Cyan, Magenta, White, Orange)
- **Surround Color:** 8 options including "Match LED Color"
- **WiFi Credentials:** Managed by WiFiManager (stored in ESP flash)

### Key Settings
- **BME280 I2C address:** 0x76 (auto-tries 0x77 if not found - see testSensor())
- **BME280 mode:** FORCED (low power, on-demand readings)
- **Web server:** Port 80
- **NTP servers:** "pool.ntp.org", "time.nist.gov" (configured in setupNTP())
- **Display rotation:** 3 (landscape 320×240)
- **WiFi AP name (setup mode):** "TFT_Clock_Setup"
- **Timezone count:** 90 global cities/regions

## Current State

### Version 2.2 (19 December 2025) - Production Ready ✅

**Fully Functional:**
- ✅ TFT display with realistic LED simulation (two styles)
- ✅ NTP time sync with 88 timezones and automatic DST
- ✅ BME280 sensor integration with graceful degradation
- ✅ Modern responsive web interface with live updates
- ✅ Three display modes with auto-cycling
- ✅ 12/24 hour format support with smart seconds display
- ✅ Display mirror feature (real-time Canvas rendering on web page)
- ✅ Dynamic environment icons based on sensor readings
- ✅ Customizable LED and surround colors (8 options each)
- ✅ WiFiManager for easy WiFi setup (no hardcoded credentials)
- ✅ REST API for status and display data

**Performance:**
- Default style: ~50 FPS (20ms refresh)
- Realistic style: ~3 FPS (300-400ms refresh)
- Memory: ~40KB used, ~40KB free
- Web updates: Live clock updates every 1 second, display mirror every 500ms

**Recent Improvements (v2.1-2.2):**
- Migrated to TFT_eSPI library (40MHz SPI, better performance)
- Complete web UI redesign (dark theme, gradient cards, responsive layout)
- Fixed LED surround color visibility issues
- Added temperature-based weather icons (7 levels)
- Added humidity-based condition icons (3 levels)
- Implemented display mirror with Canvas-based rendering
- Fixed Mode 2 leading zero issue for single-digit hours

## Architecture Notes

### Display Rendering Architecture
**Virtual LED Matrix:** Simulates 4×2 MAX7219 8×8 LED matrices (32×16 pixel display)
- **Buffer format:** 64-byte screen buffer `scr[64]` organized as packed vertical bytes
- **Buffer layout:** `scr[x + y * LINE_WIDTH]` where each byte = 8 vertical pixels
- **Physical mapping:** Each LED = 10×10 pixels on TFT (320×164 display area)
- **Authentic spacing:** 4-pixel gap between matrix rows (matches real MAX7219 hardware)

**Two Display Styles:**
1. **Default (0):** Fast solid square blocks
   - Simple fillRect calls
   - ~50 FPS refresh rate
   - Modern, clean appearance

2. **Realistic (1):** Circular LEDs with visible surrounds
   - Pixel-by-pixel circular rendering
   - Visible "off" LEDs as dark circles
   - Customizable surround/bezel colors (8 options)
   - ~3 FPS but 6-8× faster than original LED matrix version
   - Authentic MAX7219 appearance

**Fast Refresh Optimization (FAST_REFRESH flag):**
- Static 64-byte buffer tracks previous screen state
- Only redraws changed portions of display
- `forceFullRedraw` flag for instant color/style changes
- Critical for maintaining responsiveness

### Font Rendering System
**Bitmap Fonts in PROGMEM:** Five fonts stored in flash to save RAM
- `digits7x16` - Large 7×16 font (digits 0-9 and colon only)
- `digits5x16rn` - Medium 5×16 font for Mode 1 (digits 0-9 and colon only)
- `digits5x8rn` - Small 5×8 font for Mode 0 and Mode 2 (full ASCII space-to-colon range)
- `digits3x5` - Tiny 3×5 font for seconds display (digits 0-9 only)
- `font3x7` - 3×7 font for labels and text (full ASCII space-to-underscore range)

**Columnar Format:** Vertical byte packing matches LED matrix orientation
- Each font character stored as vertical columns
- Variable width support (proportional spacing)
- Helper functions: `drawChar()`, `drawCharWithY()`, `charWidth()`, `stringWidth()`

### Display Modes (Auto-cycle every 5s)
**Mode 0: Time + Temperature/Humidity**
- Top row: Time with seconds using digits5x8rn (hours/minutes) and digits3x5 (seconds)
- Bottom row: T##C H##% (using font3x7)
- Smart seconds hiding: 24-hour mode hides seconds when hours ≥ 10 (space constraint)
- Starts at x=0 (left aligned)

**Mode 1: Large Time**
- Top row: Large time display using digits5x16rn font for hours/minutes
- Seconds shown in font3x7 (not digits3x5)
- Bottom row: Empty (all display space dedicated to time)
- Dynamic positioning: Centers based on 1 or 2-digit hours
- Always shows seconds

**Mode 2: Time + Date**
- Top row: Time with seconds using digits5x8rn (hours/minutes) and digits3x5 (seconds)
- No leading zero for single-digit hours (matches Mode 0 behavior)
- Bottom row: DD/MM/YY format using font3x7
- Starts at x=2 for slight indentation
- Always shows seconds

### Time Management
- **NTP Sync:** Every hour (3600000ms) using ESP8266 built-in `configTime()` and `time.h`
- **Timezone Handling:** 90 POSIX TZ strings with automatic DST transitions (see timezones.h)
- **Format Support:** Both 12-hour and 24-hour formats (toggle via web interface)
- **Time Variables:** Maintains both `hours` (12h) and `hours24` (24h) formats
- **Time Source:** `time()` + `localtime()` called every loop iteration for second updates

### Web Server Architecture
**Single-Page Application Pattern:**
- Server-side HTML generation with embedded CSS/JavaScript
- Live updates via JavaScript fetch API (no page reload needed)
- REST API endpoints for JSON data
- Lambda-based route handlers in ESP8266WebServer

**REST API Endpoints:**
- `GET /` - Main responsive web interface (HTML/CSS/JS)
- `GET /api/time` - JSON: `{hours, minutes, seconds, day, month, year, use24hour}`
- `GET /api/display` - JSON: `{buffer[64], style, ledColor, surroundColor, width, height}`
- `GET /api/status` - JSON: `{sensor_available, temperature, humidity, pressure, temp_unit}`
- `GET /temperature?mode=toggle` - Toggle °C/°F, returns 302 redirect to `/`
- `GET /timezone?tz=0-89` - Set timezone (0-89), returns 302 redirect to `/`
- `GET /timeformat?mode=toggle` - Toggle 12/24 hour, returns 302 redirect to `/`
- `GET /style?mode=toggle` - Toggle display style (Default/Realistic)
- `GET /style?ledcolor=0-7` - Set LED color (0=Red, 1=Green, 2=Blue, 3=Yellow, 4=Cyan, 5=Magenta, 6=White, 7=Orange)
- `GET /style?surroundcolor=0-7` - Set surround color (7=Match LED Color)
- `GET /reset` - Reset WiFi credentials and restart ESP8266

**TFT Display Mirror Feature (v2.2):**
- Real-time display mirroring on web page using HTML5 Canvas
- Fetches display buffer via `/api/display` every 500ms
- Client-side rendering matches actual TFT display pixel-for-pixel
- Supports both Default and Realistic display styles
- Dynamically syncs LED color, surround color, and style settings
- Canvas size: 320×164 pixels (32×16 LEDs @ 10px each + 4px gap)
- JavaScript functions: `initCanvas()`, `drawLED()`, `updateDisplay()`
- RGB565 to RGB conversion: `rgb565ToHex()` function

**Responsive Web Design:**
- Mobile-first approach with fluid typography
- Breakpoints: Mobile (<768px), Tablet (769-1024px), Desktop (>1024px)
- Touch-friendly UI elements
- Dark theme with gradient cards and glowing effects

### Sensor Integration
**BME280 Configuration:**
- **Mode:** FORCED mode (low power - sensor sleeps between readings)
- **Sampling:** X1 for temperature, pressure, and humidity (minimal oversampling)
- **Filter:** OFF (no IIR filtering)
- **Polling:** Every 60 seconds via `takeForcedMeasurement()`
- **Address Detection:** Tries 0x76, then 0x77 if not found
- **Validation:** Range checks (temp: -50 to 100°C, humidity: 0-100%, pressure: 800-1200 hPa)
- **Graceful degradation:** If sensor not detected, displays "NO SENSOR"

**Dynamic Web UI Icons:**
- **Temperature:** 7-level weather emojis (🔥 hot ≥30°C → ❄️ freezing <5°C)
- **Humidity:** 3-level icons (💦 high >70% → 🏜️ low <30%)

### Color System
**RGB565 Format:** 16-bit color (5-bit red, 6-bit green, 5-bit blue)
- 8 predefined LED colors with full brightness
- 8 surround/bezel color options
- `dimRGB565()` function preserves hue while reducing brightness
- "Match LED Color" option syncs surround to LED color

### Memory Optimization
**PROGMEM Usage:** All font data stored in flash memory
- Saves ~8KB+ RAM by storing fonts in flash
- Critical for ESP8266's limited RAM (80KB total)
- Access via `pgm_read_byte()` function

**Buffer Efficiency:**
- Screen buffer: Only 64 bytes for 32×16 display
- Fast refresh tracking: Additional 64-byte static buffer (in refreshAll())
- Total display buffers: 128 bytes

### Important Code Patterns

**Bit Manipulation for LED Matrix:**
```cpp
// Each byte in scr[] represents 8 vertical pixels
// Setting a pixel: scr[x + y * LINE_WIDTH] |= (1 << bitPos)
// Reading a pixel: bool lit = (pixelByte & (1 << bitPos)) != 0
```

**Font Rendering Flow:**
1. `drawChar(x, char, font)` → calls `drawCharWithY(x, 0, char, font)`
2. `drawCharWithY()` reads PROGMEM font data and writes to scr[] buffer
3. Each character is rendered as vertical columns
4. Returns width of character for next character positioning

**Display Update Flow:**
1. Clear buffer: `clearScreen()` (sets all scr[] bytes to 0)
2. Draw content: `displayTimeAndTemp()` / `displayTimeLarge()` / `displayTimeAndDate()`
3. Render to TFT: `refreshAll()` (converts buffer to TFT pixels)
4. Mode switching: Automatic every 5 seconds in loop()

**Force Redraw Mechanism:**
```cpp
forceFullRedraw = true;  // Set flag
forceCompleteRefresh();  // Clear screen and buffer
displayTimeAndTemp();     // Redraw content
refreshAll();            // Render with fresh state
// forceFullRedraw flag cleared automatically in refreshAll()
```

**Circular LED Rendering (Realistic Style):**
```cpp
// Distance-squared calculation for circular shape
int dx = (cx * 2 - 9);
int dy = (cy * 2 - 9);
int distSq = dx * dx + dy * dy;

// Three concentric regions:
if (distSq <= 18)      // Inner core (bright LED color)
if (distSq <= 38)      // LED body (bright LED color)
if (distSq <= 62)      // Surround/bezel ring (full surround color)
else                   // Outside circle (black background)
```

**Off LED Rendering:**
- Visible dark circles (like real MAX7219 hardware)
- Very dim LED center (0x1800 - dark red)
- Very dim surround (ledSurroundColor dimmed by factor 7)

## Known Issues

### Display Limitations
1. **24-Hour Mode 0 Seconds:** Hidden when hours ≥ 10 due to space constraints
   - Workaround: Use Mode 1 or Mode 2, or switch to 12-hour format
   - Status: Documented feature, not a bug

2. **Realistic Style Performance:** ~300-400ms refresh vs ~20ms for default style
   - Cause: Pixel-by-pixel circular rendering algorithm
   - Status: Acceptable tradeoff for visual authenticity (still 6-8× faster than original)

3. **No Brightness Control:** Display always at full brightness (backlight on/off only)
   - PIR and LDR sensors removed from TFT version
   - Status: Could add PWM brightness control to D8 (LED_PIN) if needed

### Code Notes
**Bottom Line Shift Disabled:**
- Original MAX7219 code had bottom row bit-shift for text alignment
- Causes visual artifacts on TFT display
- Disabled at [main_tft.cpp:588](src/main_tft.cpp#L588) and [main_tft.cpp:691](src/main_tft.cpp#L691)
- Status: Intentionally disabled, not a bug

### Hardware Constraints
1. **ESP8266 RAM:** ~40KB free - sufficient but limits future expansion
2. **5GHz WiFi:** Not supported (ESP8266 limitation)
3. **Sensor Address:** BME280 must be at 0x76 or 0x77 (configurable in code)

## TODO

### High Priority
- [ ] Get weather from online API and display on matrix and webpage
  - Integration with OpenWeatherMap or similar
  - Display weather icons on LED matrix
  - Update web interface with forecast data

- [ ] Add OTA firmware update capability
  - ESP8266HTTPUpdate or ArduinoOTA
  - Web interface upload button
  - Version checking

### Display Enhancements
- [ ] Add new display modes, like morphing digits (from @cbmamiga)
  - Smooth transitions between numbers
  - Animation effects

- [ ] Optimize TFT drawing routines further for even faster refresh rates
  - Investigate DMA transfers
  - Sprite-based rendering for realistic style

- [ ] Update README.md with sample screenshots of new web interface and TFT display
  - Web interface screenshots
  - Physical display photos
  - Display style comparisons

### Future Enhancements (from README)
- [ ] Additional display animations (scrolling, fading, etc.)
- [ ] Custom font support (user-uploadable fonts)
- [ ] MQTT integration for home automation
- [ ] Alarm functionality with buzzer
- [ ] Multiple timezone clocks (world clock mode)
- [ ] Adjustable LED glow/bloom effects for realistic style

### Platform Expansion
- [ ] Refactor code for ESP32 compatibility
  - Use ESP32-specific optimizations
  - Support for CYD display: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
  - Parallel display interface option
  - More RAM for advanced features

### Code Quality
- [ ] Split main_tft.cpp into multiple files for better organization
  - display.cpp/h - Display rendering functions
  - webserver.cpp/h - Web server and API handlers
  - sensors.cpp/h - BME280 management
  - time.cpp/h - NTP and timezone handling

- [ ] Add unit tests for critical functions
  - Font rendering tests
  - Buffer manipulation tests
  - Color conversion tests

## Development Notes

### Debugging
- Serial output enabled by default (DEBUG_ENABLED flag)
- Monitor at 115200 baud
- Status printed every 10 seconds
- Detailed display update logging available

### Testing Display Styles
- Use web interface `/style?mode=toggle` to switch between styles
- Test all 8 LED colors and 8 surround colors
- Verify `forceFullRedraw` works correctly (instant color change)

### Adding Timezones
1. Edit `include/timezones.h`
2. Add POSIX TZ string to `timezones[]` array
3. Format: `{"City, Country", "TZ_STRING"}`
4. Count auto-updates via `sizeof` calculation

### TFT_eSPI Display Configuration
- Edit `include/User_Setup.h` for different display types
- Uncomment appropriate driver (#define ST7789_DRIVER for ST7789)
- Adjust pin mappings if needed
- SPI frequency can be reduced if display issues occur

### Memory Management
- Use PROGMEM for large constant data
- Minimize String usage (prefer char arrays)
- Monitor free heap: `ESP.getFreeHeap()`
- Current usage: ~40KB used, ~40KB free

### Web Interface Development
- HTML/CSS/JS embedded in C++ strings (no SPIFFS needed)
- Test responsive design at multiple screen sizes
- Use browser dev tools to debug fetch API calls
- Check `/api/*` endpoints return valid JSON

## Build and Upload Commands

```bash
# Build project
pio run

# Upload to ESP8266
pio run --target upload

# Monitor serial output
pio device monitor

# Clean build
pio run --target clean

# Build and upload
pio run --target upload && pio device monitor
```

## WiFi Setup Process

1. Flash firmware to ESP8266
2. Display shows "INIT" → "WIFI..."
3. Connect phone/computer to "TFT_Clock_Setup" AP
4. Configuration portal opens automatically
5. Select WiFi network and enter password
6. Display shows "WIFI OK" → "TIME OK" → "READY"
7. Access web interface at displayed IP address

## Credits

- Original LED Matrix version by Anthony Clarke
- TFT refactor and enhancements by Anthony Clarke
- Realistic LED rendering inspired by real MAX7219 hardware
- Based on MAX7219 concepts by Pawel A. Hernik
- Font data from LED matrix project by https://www.youtube.com/@cbm80amiga

## License

MIT License - Feel free to modify and share!
