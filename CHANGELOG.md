# Changelog

All notable changes to the ESP8266 TFT LED Retro Clock project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.2.0] - 2026-01-20

### Added
- About section in web UI footer with GitHub repo link and BlueSky contact details
- VERSION constant for centralized version management
- Inline comments in code for easy TFT Display Mirror size customization

### Changed
- Increased TFT Display Mirror size from 320×164px to 800×410px (25px per LED)
- VERSION constant now used in serial output and web UI footer
- Dynamic banner formatting to accommodate any version string length

## [1.0.0] - 2025-12-19

### Added
- TFT Display Mirror feature on web page with real-time Canvas rendering
- New `/api/display` endpoint returns 64-byte buffer for display mirroring
- Canvas-based LED matrix rendering in browser (supports both display styles)
- Auto-refresh display mirror every 500ms for near real-time updates
- Dynamically syncs LED color, surround color, and display style settings
- Completely redesigned web interface with modern dark theme
- Large digital clock display with live auto-update (updates every second)
- Dynamic temperature icons based on actual temperature readings:
  - 🔥 Hot (≥30°C)
  - ☀️ Warm (25-29°C)
  - 🌤️ Pleasant (20-24°C)
  - ⛅ Mild (15-19°C)
  - ☁️ Cool (10-14°C)
  - 🌧️ Cold (5-9°C)
  - ❄️ Freezing (<5°C)
- Dynamic humidity icons (💦 high, 💧 normal, 🏜️ low)
- Enhanced environment display with color-coded values and glowing effects
- Fully responsive web design for mobile/tablet/desktop
- Fluid typography with CSS clamp() for all screen sizes
- Responsive grid layout for environment cards
- Touch-friendly UI elements and hover effects
- Migrated to TFT_eSPI library for better performance
- Configured 40MHz SPI bus speed for faster display updates

### Fixed
- Mode 2 (Time+Date) now removes leading zero from single-digit hours
- LED surround color rendering - now properly visible and configurable
- Improved LED pixel rendering with better surround ring visibility

### Changed
- Migrated from Adafruit_GFX/Adafruit_ILI9341 to TFT_eSPI library
- Improved rendering performance with hardware-optimized library

## [0.9.0] - Pre-TFT_eSPI Migration

### Features
- Simulates 4×2 MAX7219 LED matrix appearance on TFT display
- WiFiManager for easy WiFi setup (no hardcoded credentials)
- BME280 I2C temperature/pressure/humidity sensor (optional)
- Automatic NTP time synchronization with DST support
- 88 global timezones with automatic DST transitions
- Web interface for configuration and monitoring
- Three auto-cycling display modes:
  - Mode 0: Time + Temperature/Humidity
  - Mode 1: Large Time display
  - Mode 2: Time + Date
- Two display styles: Default (fast blocks) and Realistic (circular LEDs)
- Customizable LED colors (8 options)
- Customizable surround/bezel colors (8 options)
- 12/24 hour time format toggle
- Celsius/Fahrenheit temperature toggle
- REST API endpoints for integration

[Unreleased]: https://github.com/anthonyjclarke/ESP8266_TFT_RetroClock/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/anthonyjclarke/ESP8266_TFT_RetroClock/compare/v1.0.0...v1.2.0
[1.0.0]: https://github.com/anthonyjclarke/ESP8266_TFT_RetroClock/compare/v0.9.0...v1.0.0
[0.9.0]: https://github.com/anthonyjclarke/ESP8266_TFT_RetroClock/releases/tag/v0.9.0
