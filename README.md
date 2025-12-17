# ESP8266 TFT LED Matrix Clock

A refactored version of the ESP8266 LED Matrix Clock that uses SPI TFT displays (ILI9341/ST7789) to **simulate** the appearance of a 4x MAX7219 8x8 LED matrix. All functionality from the original LED matrix version is preserved.

## Features

- 🎨 **Simulates 32x16 LED Matrix Display** on TFT screen with realistic LED pixel appearance (4x2 MAX7219 Displays)
- ⏰ **Three Display Modes**: Time+Temperature, Large Time, Time+Date
- 🌡️ **BME280 Sensor Integration**: Temperature, humidity, and pressure readings
- 📡 **WiFi Manager**: Easy setup without hardcoded credentials
- 🌐 **NTP Time Sync**: Automatic time synchronization with 88 timezone options
- 🌐 **Web Interface**: Full configuration and monitoring via browser
- 💡 **Always-On Display**: TFT backlight stays on without extra sensors

## Display Simulation

The TFT display renders individual "LED" pixels to replicate the classic LED matrix look:
- Each LED is rendered as a rounded rectangle with appropriate spacing
- "Off" LEDs are shown in very dark red to simulate the matrix appearance
- The display is centered on the TFT screen

## Hardware Requirements

### Required Components

1. **ESP8266 Development Board**
   - D1 Mini, NodeMCU, or similar
   - Minimum 80KB RAM

2. **TFT Display** (Choose ONE):
   - 1.8" ILI9341 (240x320) SPI TFT Display
   - 2.4" ILI9341 (240x320) SPI TFT Display
   - 2.8" ILI9341 (240x320) SPI TFT Display
   - 2.4" ST7789 (240x320) SPI TFT Display

3. **BME280 Sensor** (Optional but recommended)
   - I2C Temperature/Humidity/Pressure sensor
   - Default address: 0x76 (can be 0x77)

### Optional Components
- Breadboard or PCB
- Jumper wires
- 5V power supply (1A minimum)

## Wiring Diagram

### TFT Display (ILI9341/ST7789) - SPI Connection

```
ESP8266 D1 Mini  →  TFT Display
─────────────────────────────────
D8 (GPIO15)      →  CS  (Chip Select)
D3 (GPIO0)       →  DC  (Data/Command)
D4 (GPIO2)       →  RST (Reset)
D7 (GPIO13)      →  MOSI/SDA (Data)
D5 (GPIO14)      →  SCK/SCL (Clock)
3.3V             →  VCC
3.3V             →  LED (Backlight) *
GND              →  GND

* Note: Some TFT modules have built-in backlight control.
  If your module has a separate LED pin, connect it to 3.3V
  to keep the backlight permanently on.
```

### BME280 Sensor - I2C Connection

```
ESP8266 D1 Mini  →  BME280
────────────────────────────
D2 (GPIO4)       →  SDA
D1 (GPIO5)       →  SCL
3.3V             →  VCC
GND              →  GND
```

### Complete Wiring Table

| Component      | Pin Name  | ESP8266 Pin | GPIO | Notes                    |
|----------------|-----------|-------------|------|--------------------------|
| **TFT Display**|           |             |      |                          |
| CS             | TFT_CS    | D8          | 15   | Chip Select              |
| DC             | TFT_DC    | D3          | 0    | Data/Command             |
| RST            | TFT_RST   | D4          | 2    | Reset                    |
| MOSI           | MOSI      | D7          | 13   | Hardware SPI             |
| SCK            | SCK       | D5          | 14   | Hardware SPI             |
| **BME280**     |           |             |      |                          |
| SDA            | SDA       | D2          | 4    | I2C Data                 |
| SCL            | SCL       | D1          | 5    | I2C Clock                |

## Display Configuration

### Supported Display Types

The code supports both ILI9341 and ST7789 displays. Choose your display type in `main_tft.cpp`:

```cpp
// Uncomment ONE of the following:
#define USE_ILI9341    // For ILI9341 displays (most common)
// #define USE_ST7789  // For ST7789 displays
```

### Display Customization

You can adjust the LED appearance by modifying these constants in `main_tft.cpp`:

```cpp
#define LED_SIZE          8      // Size of each LED pixel (pixels)
#define LED_SPACING       2      // Space between LEDs (pixels)
#define LED_COLOR         0xF800 // LED color (RGB565: Red)
#define BG_COLOR          0x0000 // Background (Black)
#define LED_OFF_COLOR     0x1082 // "Off" LED color (very dark red)
```

### Color Options (RGB565 format)

```cpp
// Common colors in RGB565 format:
0xF800  // Red
0x07E0  // Green
0x001F  // Blue
0xFFE0  // Yellow
0xF81F  // Magenta
0x07FF  // Cyan
0xFFFF  // White
0x0000  // Black
0xF7BE  // Orange
```

## Software Setup

### 1. Install PlatformIO

Install PlatformIO IDE (VS Code extension) or PlatformIO Core:
- **VS Code**: Install "PlatformIO IDE" extension
- **Command Line**: `pip install platformio`

### 2. Clone/Download Project

```bash
git clone <your-repo-url>
cd tft-led-clock
```

### 3. Configure Display Type

Edit `main_tft.cpp` and select your display:

```cpp
#define USE_ILI9341    // Most common 2.4" and 2.8" displays
// #define USE_ST7789  // Some 1.8" and 2.4" displays
```

### 4. Verify Pin Connections

Check that the pin definitions in `main_tft.cpp` match your wiring:

```cpp
#define TFT_CS    D8
#define TFT_DC    D3
#define TFT_RST   D4
#define SDA_PIN   D2
#define SCL_PIN   D1
```

### 5. Build and Upload

Using PlatformIO:

```bash
# Build the project
pio run

# Upload to ESP8266
pio run --target upload

# Monitor serial output
pio device monitor
```

Or use the PlatformIO IDE buttons in VS Code.

### 6. Initial WiFi Setup

1. Power on the ESP8266
2. Look for WiFi network: **TFT_Clock_Setup**
3. Connect to it with your phone/computer
4. Browser should auto-open to configuration portal
5. Select your WiFi network and enter password
6. Click "Save"

The display will show status messages during setup:
- `INIT` - Initializing display
- `WIFI OK` - WiFi connected
- `TIME OK` - NTP sync successful
- `READY` - System ready

## Web Interface

After WiFi setup, access the web interface:

```
http://<esp8266-ip-address>
```

Find the IP address from:
- Serial monitor output
- Your router's DHCP client list
- mDNS: `http://tft-clock.local` (if mDNS is working)

### Web Interface Features

- **Current Time & Environment**: Real-time clock and sensor data
- **Settings**: Timezone picklist and Celsius/Fahrenheit toggle
- **System Tools**: Shows IP/uptime and lets you reset WiFi credentials

## Display Modes

The clock automatically cycles through three display modes every 5 seconds:

1. **Time + Temperature/Humidity** (Mode 0)
   - Large time display (HH:MM)
   - Temperature and humidity on the right
   - Colon blinks every second

2. **Large Time** (Mode 1)
   - Centered large time display
   - Maximum visibility

3. **Time + Date** (Mode 2)
   - Time on top row
   - Date (DD/MM) on bottom row

## Configuration Options

### Brightness Control

### Timezone Configuration

88 supported timezones including:
- All Australian zones (Sydney, Melbourne, Brisbane, Adelaide, Perth, Darwin, Hobart)
- US zones (EST, CST, MST, PST, Alaska, Hawaii)
- European zones (UK, CET, EET)
- Asian zones (Tokyo, Hong Kong, Singapore, Bangkok, Mumbai, Dubai)
- And many more...

## Troubleshooting

### Display Issues

**Display is blank:**
- Check TFT connections (CS, DC, RST, MOSI, SCK)
- Verify correct display type is selected (#define)
- Check TFT power supply (3.3V)
- Check rotation setting: `tft.setRotation(1)`

**Display shows garbage:**
- Wrong display type selected
- SPI pins connected incorrectly
- Try different rotation values (0-3)

**LEDs look wrong:**
- Adjust LED_SIZE, LED_SPACING constants
- Change LED_COLOR to preferred color
- Modify LED_OFF_COLOR for better contrast

### Sensor Issues

**BME280 not detected:**
- Check I2C connections (SDA, SCL)
- Verify I2C address (0x76 or 0x77)
- Try other address in code: `bme280.begin(0x77)`
- Check I2C pullup resistors (may be required)

### WiFi Issues

**Won't connect to WiFi:**
- Hold button/power cycle to reset WiFi settings
- Connect to "TFT_Clock_Setup" AP and reconfigure
- Check WiFi credentials are correct
- Ensure 2.4GHz network (ESP8266 doesn't support 5GHz)

**Web interface not accessible:**
- Check ESP8266 IP address in serial monitor
- Verify ESP8266 and computer on same network
- Try accessing directly by IP: `http://192.168.x.x`

### Time Issues

**Time not syncing:**
- Check WiFi connection
- Verify firewall allows NTP (port 123 UDP)
- Try different NTP server in code
- Check timezone selection

**Wrong timezone:**
- Select correct timezone in web interface
- Verify POSIX TZ string is correct
- Check DST settings for your region

## Serial Debug Output

Connect to serial monitor (115200 baud) to see:

```
╔════════════════════════════════════════╗
║   ESP8266 TFT Matrix Clock v1.0        ║
║   TFT Display Edition                  ║
╚════════════════════════════════════════╝

Initializing TFT Display...
TFT Display initialized: 320x240
LED Matrix area: 258x82 at offset (31,79)
BME280 OK: 22.5°C, 45.3%
Connected! IP: 192.168.1.100
Syncing time with NTP...
Time synced: 14:23:45 17/12/2025 (TZ: Sydney, Australia)
Web server started

Time: 14:23 | Temp: 23°C | Hum: 45% | Bright: 8 | Light: 512 | Motion: NO | Display: ON
```

## Advanced Configuration

### Custom LED Colors

Create your own color schemes:

```cpp
// Retro green terminal look
#define LED_COLOR       0x07E0  // Green
#define LED_OFF_COLOR   0x0208  // Very dark green

// Blue ice look
#define LED_COLOR       0x001F  // Blue
#define LED_OFF_COLOR   0x0004  // Very dark blue

// Amber/orange look
#define LED_COLOR       0xFD20  // Amber
#define LED_OFF_COLOR   0x2000  // Very dark amber
```

### Adjust LED Appearance

```cpp
// Larger LEDs with more spacing (vintage look)
#define LED_SIZE          10
#define LED_SPACING       3

// Smaller, denser LEDs (modern look)
#define LED_SIZE          6
#define LED_SPACING       1

// Square LEDs (remove rounded corners)
// In drawLEDPixel() function:
tft.fillRect(screenX, screenY, LED_SIZE, LED_SIZE, color);  // Instead of fillRoundRect
```

### Display Rotation

Adjust display orientation:

```cpp
// In initTFT() function:
tft.setRotation(0);  // Portrait
tft.setRotation(1);  // Landscape (default)
tft.setRotation(2);  // Portrait inverted
tft.setRotation(3);  // Landscape inverted
```

## API Endpoints

The web server provides JSON APIs:

### GET /api/time
Returns current time:
```json
{
  "hours": 14,
  "minutes": 23,
  "seconds": 45,
  "day": 17,
  "month": 12,
  "year": 2025
}
```

### GET /api/status
Returns system status:
```json
{
  "sensor_available": true,
  "temperature": 23,
  "humidity": 45,
  "pressure": 1013,
  "temp_unit": "Celsius"
}
```

### POST /temperature
Toggle temperature unit:
- `mode=toggle` - Toggle °C/°F

### POST /timezone
Change timezone:
- `tz=0-87` - Set timezone index

## Performance Notes

- **Refresh Rate**: ~10 FPS for smooth updates
- **Power Consumption**: ~250mA @ 5V with backlight always on
- **WiFi**: 2.4GHz only (ESP8266 limitation)
- **Memory**: ~35KB RAM used, ~45KB free

## Future Enhancements

- [ ] Weather API integration
- [ ] OTA firmware updates
- [ ] Multiple display animations
- [ ] Custom font support
- [ ] MQTT integration
- [ ] Touchscreen support
- [ ] Alarm functionality
- [ ] Multiple timezone clocks

## Credits

- Original LED Matrix version by Anthony Clarke
- Based on MAX7219 concepts by Pawel A. Hernik
- TFT refactor maintains all original functionality
- Font data from original LED matrix project

## License

MIT License - Feel free to modify and share!

## Support

For issues, questions, or contributions:
- Open an issue on GitHub
- Check the troubleshooting section above
- Review serial debug output for diagnostics

---

**Enjoy your new TFT LED Matrix Clock!** 🎨⏰
