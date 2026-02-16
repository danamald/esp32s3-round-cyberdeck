/*
 * CYBERDECK - ROUND DISPLAY EDITION
 * Waveshare ESP32-S3 1.28" Round Display (240x240)
 * Speedometer-style gauges with rotating needles
 */

#include <WiFi.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>
#include <CST816S.h>

// WiFi Configuration
#define WIFI_SSID "Davis"
#define WIFI_PASSWORD "greenbreeze713"
#define PI_IP_ADDRESS "192.168.1.180"
#define PI_PORT 8888

// Display pins for Waveshare 1.28" Round
#define TFT_SCLK 10
#define TFT_MOSI 11
#define TFT_MISO 13
#define TFT_CS   9
#define TFT_DC   8
#define TFT_RST  14
#define TFT_BL   2

// Touch pins
#define TP_SDA 6
#define TP_SCL 7
#define TP_INT 5
#define TP_RST 13

// Colors
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW  0xFFE0
#define COLOR_ORANGE  0xFD20
#define COLOR_DARKGRAY 0x7BEF

// Display setup
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

// Touch setup
CST816S touch(TP_SDA, TP_SCL, TP_RST, TP_INT);

WiFiClient client;

// Display parameters
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define CENTER_X 120
#define CENTER_Y 120

// Pages
#define NUM_PAGES 4
int currentPage = 0;
unsigned long lastPageChange = 0;
#define PAGE_CHANGE_DELAY 300

// Data structure
struct MonitorData {
  float battery_percent;
  float battery_voltage;
  float battery_current;
  float cpu_temp;
  float memory_percent;
  String time;
  String date;
  bool valid;
} currentData;

// Touch tracking
int16_t touchStartX = 0;
#define SWIPE_THRESHOLD 30

void setup() {
  Serial.begin(115200);
  Serial.println("Cyberdeck Round Display Starting...");
  
  // Backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // Initialize display
  if (!gfx->begin()) {
    Serial.println("Display init failed!");
  }
  gfx->fillScreen(COLOR_BLACK);
  
  // Show startup
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR_CYAN);
  gfx->setCursor(30, 100);
  gfx->println("CYBERDECK");
  gfx->setTextSize(1);
  gfx->setCursor(60, 130);
  gfx->println("ROUND EDITION");
  
  // Initialize touch
  touch.begin();
  
  // Set default data
  currentData.battery_percent = 100;
  currentData.battery_voltage = 16.40;
  currentData.battery_current = 0;
  currentData.cpu_temp = 59.0;
  currentData.memory_percent = 3.0;
  currentData.time = "18:21:25";
  currentData.date = "2026-02-15";
  currentData.valid = true;
  
  delay(1000);
  
  // Connect WiFi
  gfx->setCursor(40, 150);
  gfx->println("Connecting WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    gfx->setTextColor(COLOR_GREEN);
    gfx->setCursor(60, 170);
    gfx->println("WiFi OK!");
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Try Pi connection
    Serial.println("Attempting Pi connection...");
    if (client.connect(PI_IP_ADDRESS, PI_PORT)) {
      gfx->setCursor(50, 190);
      gfx->println("Pi Connected!");
      Serial.println("Pi connected successfully!");
    } else {
      gfx->setTextColor(COLOR_YELLOW);
      gfx->setCursor(50, 190);
      gfx->println("Pi offline");
      Serial.println("Pi connection FAILED - check IP and server running");
    }
  } else {
    Serial.println("WiFi connection FAILED");
  }
  
  delay(1000);
  drawCurrentPage();
}

void loop() {
  // Handle touch
  if (touch.available()) {
    int x = touch.data.x;
    
    if (touchStartX == 0) {
      touchStartX = x;
    } else {
      int deltaX = x - touchStartX;
      if (abs(deltaX) > SWIPE_THRESHOLD) {
        changePage(deltaX > 0 ? -1 : 1);
        touchStartX = 0;
      }
    }
  } else {
    touchStartX = 0;
  }
  
  // Receive data from Pi
  if (client.connected() && client.available()) {
    String jsonString = client.readStringUntil('\n');
    if (jsonString.length() > 0) {
      Serial.print("Received data: ");
      Serial.println(jsonString);
      parseData(jsonString);
    }
  } else if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    // Try to reconnect if disconnected
    static unsigned long lastReconnect = 0;
    if (millis() - lastReconnect > 10000) {  // Try every 10 seconds
      Serial.println("Attempting to reconnect to Pi...");
      if (client.connect(PI_IP_ADDRESS, PI_PORT)) {
        Serial.println("Reconnected to Pi!");
      }
      lastReconnect = millis();
    }
  }
  
  // Redraw every 2 seconds
  static unsigned long lastDraw = 0;
  if (millis() - lastDraw > 2000) {
    lastDraw = millis();
    drawCurrentPage();
  }
  
  delay(50);
}

void changePage(int direction) {
  if (millis() - lastPageChange < PAGE_CHANGE_DELAY) return;
  currentPage += direction;
  if (currentPage < 0) currentPage = NUM_PAGES - 1;
  if (currentPage >= NUM_PAGES) currentPage = 0;
  lastPageChange = millis();
  drawCurrentPage();
}

void parseData(String jsonString) {
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, jsonString)) return;
  
  currentData.battery_percent = doc["battery_percent"];
  currentData.battery_voltage = doc["battery_voltage"];
  currentData.battery_current = doc["battery_current"];
  currentData.cpu_temp = doc["cpu_temp"];
  currentData.memory_percent = doc["memory_percent"];
  currentData.time = doc["time"].as<String>();
  currentData.date = doc["date"].as<String>();
  currentData.valid = true;
}

void drawCurrentPage() {
  gfx->fillScreen(COLOR_BLACK);
  
  switch(currentPage) {
    case 0: drawBatteryGauge(); break;
    case 1: drawCPUGauge(); break;
    case 2: drawMemoryGauge(); break;
    case 3: drawClockPage(); break;
  }
  
  drawPageDots();
}

// ===== BATTERY GAUGE =====
void drawBatteryGauge() {
  drawCircularGauge(
    "BATTERY",
    currentData.battery_percent,
    0, 100,
    "%",
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_RED,
    50, 20
  );
  
  // Voltage and current at bottom
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_WHITE);
  char buf[20];
  sprintf(buf, "%.2fV  %.0fmA", currentData.battery_voltage, currentData.battery_current);
  int tw = strlen(buf) * 6;
  gfx->setCursor(CENTER_X - tw/2, 200);
  gfx->println(buf);
}

// ===== CPU TEMPERATURE GAUGE =====
void drawCPUGauge() {
  drawCircularGauge(
    "CPU TEMP",
    currentData.cpu_temp,
    0, 100,
    "C",
    COLOR_GREEN,
    COLOR_ORANGE,
    COLOR_RED,
    60, 75
  );
}

// ===== MEMORY GAUGE =====
void drawMemoryGauge() {
  drawCircularGauge(
    "MEMORY",
    currentData.memory_percent,
    0, 100,
    "%",
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_RED,
    50, 75
  );
}

// ===== CIRCULAR GAUGE DRAWING =====
void drawCircularGauge(const char* title, float value, float minVal, float maxVal, 
                       const char* unit, uint16_t colorLow, uint16_t colorMid, uint16_t colorHigh,
                       float warnThreshold, float criticalThreshold) {
  
  // Title at top
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_CYAN);
  int tw = strlen(title) * 6;
  gfx->setCursor(CENTER_X - tw/2, 20);
  gfx->println(title);
  
  // Draw gauge arc (270 degrees, -135 to +135)
  int radius = 90;
  float startAngle = -135;
  float endAngle = 135;
  float totalDegrees = endAngle - startAngle;
  
  // Draw tick marks and scale
  for (int i = 0; i <= 10; i++) {
    float angle = startAngle + (totalDegrees * i / 10.0);
    float rad = angle * PI / 180.0;
    
    int x1 = CENTER_X + cos(rad) * (radius - 10);
    int y1 = CENTER_Y + sin(rad) * (radius - 10);
    int x2 = CENTER_X + cos(rad) * radius;
    int y2 = CENTER_Y + sin(rad) * radius;
    
    gfx->drawLine(x1, y1, x2, y2, COLOR_DARKGRAY);
    
    // Scale numbers
    if (i % 2 == 0) {
      int scaleVal = minVal + (maxVal - minVal) * i / 10.0;
      char numBuf[5];
      sprintf(numBuf, "%d", scaleVal);
      int nx = CENTER_X + cos(rad) * (radius - 20) - 6;
      int ny = CENTER_Y + sin(rad) * (radius - 20) - 4;
      gfx->setTextColor(COLOR_WHITE);
      gfx->setCursor(nx, ny);
      gfx->print(numBuf);
    }
  }
  
  // Draw colored arc sections
  float percent = (value - minVal) / (maxVal - minVal) * 100.0;
  
  for (float a = startAngle; a < endAngle; a += 2) {
    float currentPercent = (a - startAngle) / totalDegrees * 100.0;
    uint16_t color;
    
    if (currentPercent < warnThreshold) {
      color = colorLow;
    } else if (currentPercent < criticalThreshold) {
      color = colorMid;
    } else {
      color = colorHigh;
    }
    
    if (currentPercent <= percent) {
      float rad = a * PI / 180.0;
      int x1 = CENTER_X + cos(rad) * (radius - 8);
      int y1 = CENTER_Y + sin(rad) * (radius - 8);
      int x2 = CENTER_X + cos(rad) * (radius - 2);
      int y2 = CENTER_Y + sin(rad) * (radius - 2);
      
      gfx->drawLine(x1, y1, x2, y2, color);
    }
  }
  
  // Draw needle
  float needleAngle = startAngle + (totalDegrees * percent / 100.0);
  drawNeedle(needleAngle, radius - 15, COLOR_WHITE);
  
  // Center circle
  gfx->fillCircle(CENTER_X, CENTER_Y, 8, COLOR_WHITE);
  gfx->fillCircle(CENTER_X, CENTER_Y, 6, COLOR_BLACK);
  
  // Value display in center
  gfx->setTextSize(3);
  gfx->setTextColor(COLOR_WHITE);
  char valBuf[10];
  sprintf(valBuf, "%.0f", value);
  tw = strlen(valBuf) * 18;
  gfx->setCursor(CENTER_X - tw/2, CENTER_Y + 20);
  gfx->println(valBuf);
  
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_CYAN);
  tw = strlen(unit) * 6;
  gfx->setCursor(CENTER_X - tw/2, CENTER_Y + 45);
  gfx->println(unit);
}

// Draw needle (simple line method)
void drawNeedle(float angle, int length, uint16_t color) {
  float rad = angle * PI / 180.0;
  int x = CENTER_X + cos(rad) * length;
  int y = CENTER_Y + sin(rad) * length;
  
  // Draw thick needle
  for (int i = -2; i <= 2; i++) {
    gfx->drawLine(CENTER_X + i, CENTER_Y, x + i, y, color);
    gfx->drawLine(CENTER_X, CENTER_Y + i, x, y + i, color);
  }
}

// ===== CLOCK PAGE =====
void drawClockPage() {
  // Title
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_CYAN);
  gfx->setCursor(95, 20);  // Centered (was 85)
  gfx->println("CLOCK");
  
  // Parse time
  int h = currentData.time.substring(0, 2).toInt();
  int m = currentData.time.substring(3, 5).toInt();
  int s = currentData.time.substring(6, 8).toInt();
  
  // Draw clock face
  int clockRadius = 90;
  gfx->drawCircle(CENTER_X, CENTER_Y, clockRadius, COLOR_WHITE);
  gfx->drawCircle(CENTER_X, CENTER_Y, clockRadius - 1, COLOR_WHITE);
  
  // Hour markers
  for (int i = 0; i < 12; i++) {
    float angle = (i * 30 - 90) * PI / 180.0;
    int x1 = CENTER_X + cos(angle) * (clockRadius - 10);
    int y1 = CENTER_Y + sin(angle) * (clockRadius - 10);
    int x2 = CENTER_X + cos(angle) * clockRadius;
    int y2 = CENTER_Y + sin(angle) * clockRadius;
    gfx->drawLine(x1, y1, x2, y2, COLOR_WHITE);
  }
  
  // Hour hand
  float hourAngle = ((h % 12) * 30 + m * 0.5 - 90);
  drawNeedle(hourAngle, 40, COLOR_WHITE);
  
  // Minute hand
  float minAngle = (m * 6 - 90);
  drawNeedle(minAngle, 60, COLOR_CYAN);
  
  // Second hand (thin)
  float secAngle = (s * 6 - 90);
  float rad = secAngle * PI / 180.0;
  int sx = CENTER_X + cos(rad) * 70;
  int sy = CENTER_Y + sin(rad) * 70;
  gfx->drawLine(CENTER_X, CENTER_Y, sx, sy, COLOR_RED);
  
  // Center dot
  gfx->fillCircle(CENTER_X, CENTER_Y, 5, COLOR_WHITE);
  
  // Digital time ABOVE center
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR_GREEN);
  int tw = currentData.time.length() * 12;
  gfx->setCursor(CENTER_X - tw/2, CENTER_Y - 40);  // Moved above center (was 200 at bottom)
  gfx->println(currentData.time);
}

// ===== PAGE DOTS =====
void drawPageDots() {
  int y = 230;
  int spacing = 12;
  int startX = CENTER_X - (NUM_PAGES * spacing / 2);
  
  for (int i = 0; i < NUM_PAGES; i++) {
    int x = startX + i * spacing;
    if (i == currentPage) {
      gfx->fillCircle(x, y, 4, COLOR_CYAN);
    } else {
      gfx->drawCircle(x, y, 3, COLOR_DARKGRAY);
    }
  }
}
