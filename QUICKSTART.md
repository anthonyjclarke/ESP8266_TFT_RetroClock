# Quick Start Guide - TFT LED Matrix Clock

## 🚀 Get Up and Running in 15 Minutes

This guide will get your TFT LED Matrix Clock working quickly. For detailed information, see README_TFT.md.

## 📦 What You Need

### Minimum Hardware (Clock Only)
- ESP8266 D1 Mini
- ILI9341 or ST7789 TFT Display (1.8"-2.8")
- USB cable for programming
- 5V power supply

### Full Feature Hardware
- ESP8266 D1 Mini
- TFT Display (ILI9341/ST7789)
- BME280 sensor (temp/humidity)
- Breadboard + jumper wires

## 🔌 Quick Wiring

### Minimum Setup (Clock Only)

Connect TFT to ESP8266:
```
TFT → ESP8266
─────────────
CS  → D8
DC  → D3
RST → D4
MOSI→ D7
SCK → D5
VCC → 3.3V
GND → GND
LED → 3.3V (backlight)
```

That's it! You now have a working clock.

### Full Setup (All Features)

Add these connections for full functionality:

**BME280 Sensor:**
```
BME280 → ESP8266
────────────────
SDA → D2
SCL → D1
VCC → 3.3V
GND → GND
```

## 💻 Software Setup

### 1. Install PlatformIO

**Option A: VS Code (Recommended)**
1. Install [VS Code](https://code.visualstudio.com/)
2. Install "PlatformIO IDE" extension
3. Restart VS Code

**Option B: Command Line**
```bash
pip install platformio
```

### 2. Prepare the Code

1. Copy these files to your project folder:
   - `main_tft.cpp` → rename to `src/main.cpp`
   - `fonts.h` → copy to `src/fonts.h`
   - `platformio.ini` → copy to project root

2. Edit `src/main.cpp` - Choose your display type:
```cpp
// Line ~35: Uncomment ONE display type
#define USE_ILI9341    // ← Most common (2.4", 2.8")
// #define USE_ST7789  // ← Some 1.8" displays
```

3. Verify pin connections match your wiring (lines ~45-55)

### 3. Upload to ESP8266

**Using VS Code:**
- Click "Upload" button (→ icon) in PlatformIO toolbar
- Or press `Ctrl+Alt+U` (Windows/Linux) / `Cmd+Alt+U` (Mac)

**Using Command Line:**
```bash
pio run --target upload
```

## 📱 First Time Setup

### 1. Power On
- TFT should show "INIT"
- LED should display startup messages

### 2. Connect to WiFi
1. Look for WiFi network: **TFT_Clock_Setup**
2. Connect with phone/computer
3. Browser opens automatically (or go to 192.168.4.1)
4. Select your WiFi network
5. Enter password
6. Click "Save"

### 3. Watch the Display
You'll see these messages:
- `INIT` → Initializing
- `WIFI OK` → Connected
- `TIME OK` → Time synchronized
- `READY` → System ready
- Clock starts showing time!

## 🌐 Access Web Interface

### Find Your IP Address

**Method 1: Serial Monitor**
```bash
pio device monitor
```
Look for: `Connected! IP: 192.168.x.x`

**Method 2: Router**
- Check DHCP client list
- Look for device named "ESP_xxxxxx"

**Method 3: Try mDNS**
```
http://tft-clock.local
```

### Open Web Interface

Go to: `http://192.168.x.x` (use your IP)

You'll see:
- Current time and date
- Temperature and humidity (if BME280 connected)
- Quick controls for timezone and temperature unit

## ⚙️ Basic Configuration

### Set Your Timezone

1. In web interface, find "Timezone" dropdown
2. Select your city/timezone
3. Time updates automatically
4. No page reload needed!

Common timezones:
- Sydney/Melbourne/Brisbane (AEST)
- New York (EST)
- Los Angeles (PST)
- London (GMT/BST)
- Tokyo (JST)

## 🎨 Customize Appearance

### Change LED Color

Edit `main_tft.cpp` (line ~78):
```cpp
#define LED_COLOR  0xF800  // Red (default)

// Try these:
#define LED_COLOR  0x07E0  // Green
#define LED_COLOR  0x001F  // Blue
#define LED_COLOR  0xFFE0  // Yellow
#define LED_COLOR  0xF81F  // Magenta
```

Re-upload code after changing.

### Adjust LED Size

Edit `main_tft.cpp` (line ~75):
```cpp
#define LED_SIZE     8  // Pixel size
#define LED_SPACING  2  // Space between
```

Bigger = more vintage look  
Smaller = more modern look

## 🔧 Troubleshooting

### Display is Blank
- ✅ Check wiring (CS, DC, RST, MOSI, SCK)
- ✅ Verify correct display type selected
- ✅ Check 3.3V power to TFT
- ✅ Try: `tft.setRotation(0)` instead of `(1)`

### Display Shows Garbage
- ✅ Wrong display type - try other #define
- ✅ Check SPI connections
- ✅ Verify TFT is 3.3V compatible

### WiFi Not Connecting
- ✅ Reset WiFi: Power cycle or reset button
- ✅ Reconnect to "TFT_Clock_Setup" network
- ✅ Check 2.4GHz network (not 5GHz)

### Time Not Showing
- ✅ Check WiFi connection
- ✅ Wait 20 seconds for NTP sync
- ✅ Check serial monitor for errors
- ✅ Verify timezone is set correctly

### Sensor Not Working
- ✅ BME280: Check I2C wiring (SDA, SCL)
- ✅ Try address 0x77 if 0x76 fails
- ✅ Check serial monitor: "BME280 OK" message

### LEDs Look Wrong
- ✅ Try different LED_COLOR value
- ✅ Modify LED_OFF_COLOR for contrast

## 📊 Status LEDs

While running, the clock cycles through 3 display modes every 5 seconds:

1. **Time + Temperature** - Shows HH:MM and temp/humidity
2. **Large Time** - Big centered time display
3. **Time + Date** - Time on top, date on bottom

## 🎯 Next Steps

Once basic setup works:

1. **Add sensors** - Install BME280 for temp/humidity
2. **Customize** - Change colors, sizes, modes
3. **Web control** - Explore all web interface features

## 📚 More Information

- Full documentation: `README_TFT.md`
- Comparison with LED: `COMPARISON.md`
- Detailed wiring: See README_TFT.md "Wiring Diagram" section
- API documentation: See README_TFT.md "API Endpoints" section

## 🆘 Getting Help

Check serial monitor output:
```bash
pio device monitor
```

Serial output shows:
- Display initialization
- WiFi status and IP address
- NTP sync status
- Sensor readings
- Any errors

## ✅ Success Checklist

- [ ] TFT display shows "INIT" on power up
- [ ] WiFi connected (display shows "WIFI OK")
- [ ] Time synchronized (display shows "TIME OK")
- [ ] Clock displays current time
- [ ] Web interface accessible
- [ ] Can change timezone in web interface
- [ ] (Optional) Temperature/humidity showing
- [ ] (Optional) Temperature displayed in °F if desired

## 🎉 You're Done!

Your TFT LED Matrix Clock is now running!

Enjoy your retro-style clock with modern TFT technology.

---

**Remember:** The display simulates LED matrix appearance while giving you the flexibility of a full-color TFT screen. You get the best of both worlds! 🎨⏰
