[README-esp32-watch.md](https://github.com/user-attachments/files/25351784/README-esp32-watch.md)
# ⌚ ESP32-S3 Round Display Cyberdeck

A cyberpunk-style system monitor with circular speedometer gauges on the Waveshare ESP32-S3 1.28" Round Touch LCD. Connects to a Raspberry Pi over WiFi to display live system stats with animated rotating needles.

## Hardware

| Component | Model |
|-----------|-------|
| Board | Waveshare ESP32-S3 Touch LCD 1.28" |
| Display | 1.28" Round LCD, GC9A01 driver, 240×240 |
| Touch | CST816S capacitive touch |
| MCU | ESP32-S3 with WiFi/BLE |
| Interface | USB-C |

## What It Does

Four swipeable gauge pages with real-time data from a Raspberry Pi:

### 🔋 Page 1 — Battery Gauge
Circular speedometer-style needle showing battery percentage. Green/yellow/red arc with voltage and current readout at the bottom.

### 🌡️ Page 2 — CPU Temperature
Analog thermometer-style gauge with needle sweeping 0–100°C. Color-coded danger zones with large temperature reading in center.

### 💾 Page 3 — Memory Usage
RAM usage displayed as a speedometer needle with percentage in center and color-coded zones.

### 🕐 Page 4 — Analog Clock
Classic watch face with hour, minute, and second hands rotating smoothly. Military time displayed above the center point.

Swipe left/right on the touch screen to switch between pages.

## Arduino IDE Setup

### Board Settings (Tools Menu)

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240MHz (WiFi) |
| Core Debug Level | None |
| USB DFU On Boot | Disabled |
| Erase All Flash | Disabled |
| Events Run On | Core 1 |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB (128Mb) |
| JTAG Adapter | Disabled |
| Arduino Runs On | Core 1 |
| USB Firmware MSC On Boot | Disabled |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| **PSRAM** | **Disabled** |
| Upload Mode | UART0 / Hardware CDC |
| Upload Speed | 921600 |
| USB Mode | USB-OTG (TinyUSB) |

**Important notes:**
- Use **ESP32S3 Dev Module**, NOT the Waveshare board definition (Waveshare's board definition doesn't expose USB CDC options)
- **PSRAM must be Disabled** — the round display doesn't have PSRAM connected and will fail to boot with it enabled
- If upload fails, hold the **BOOT** button while clicking Upload

### Required Libraries

Install via Arduino IDE → Tools → Manage Libraries:

- **Arduino_GFX** — Graphics library for GC9A01 display driver
- **CST816S** — Touch screen driver

## Installation

1. Open `cyberdeck_round_gauges.ino` in Arduino IDE
2. Update WiFi credentials in the code:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
3. Update the Raspberry Pi IP address:
   ```cpp
   const char* piHost = "192.168.1.XXX";
   ```
4. Set the board settings from the table above
5. Select your COM port
6. Click Upload
7. On first boot, the display shows "WiFi OK" → "Pi Connected!"

## Raspberry Pi Server

The ESP32 connects to a monitoring service running on the Pi that serves system stats (CPU temp, memory usage, battery status). The service runs as a systemd unit:

```bash
# Check status
sudo systemctl status cyberdeck

# Start the service
sudo systemctl start cyberdeck

# View live logs
sudo journalctl -u cyberdeck -f
```

## Troubleshooting

| Issue | Fix |
|-------|-----|
| PSRAM errors on boot | Set PSRAM to "Disabled" in Tools |
| Serial Monitor blank | Normal with TinyUSB mode — check display for status messages instead |
| Won't upload | Hold BOOT button while clicking Upload |
| "Pi connection FAILED" | Check Pi server is running: `sudo systemctl status cyberdeck` |
| Display blank | Check USB-C connection, try different cable |
| Touch not responding | CST816S library may need reinstalling |

## Photos

*Round display showing speedometer-style CPU temperature gauge with rotating needle*

## Part of the Neighborhood Intel Station

This project is part of the [Neighborhood Intel Station](https://danamald.github.io/neighborhood-intel-station/) ecosystem — a multi-source RF intelligence dashboard combining NOAA satellite imagery, weather station data, Meshtastic mesh networking, and system monitoring.

## License

MIT
