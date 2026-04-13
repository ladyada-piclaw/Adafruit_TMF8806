/*
 * TMF8806 Time-of-Flight TFT Demo for ESP32-S2 TFT Feather
 *
 * Big, bold display for product photos/video.
 * Distance bar gauge + large mm readout.
 *
 * Written by Limor 'ladyada' Fried with assistance from Claude Code
 * Copyright 2026 Adafruit Industries, MIT license
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_TMF8806.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

Adafruit_TMF8806 tof;
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define SCREEN_W 240
#define SCREEN_H 135

#define COLOR_ORANGE 0xFD20

GFXcanvas16 canvas(SCREEN_W, SCREEN_H);

// Layout
#define TITLE_Y 22
#define BAR_Y 32
#define BAR_H 30
#define BAR_MARGIN 4
#define DIST_Y 110

static uint16_t distMax = 2500; // auto-scales up

uint16_t distanceColor(uint16_t distance, uint8_t reliability) {
  if (reliability == 0) {
    return ST77XX_RED;
  }
  if (distance <= 500) {
    return ST77XX_GREEN;
  } else if (distance <= 1500) {
    return ST77XX_YELLOW;
  } else if (distance <= 3000) {
    return COLOR_ORANGE;
  }
  return ST77XX_RED;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(100);

  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  display.init(135, 240);
  display.setRotation(3);

  canvas.setTextWrap(false);

  if (!tof.begin()) {
    canvas.fillScreen(ST77XX_BLACK);
    canvas.setFont(&FreeSansBold18pt7b);
    canvas.setTextColor(ST77XX_RED);
    canvas.setCursor(6, 80);
    canvas.print(F("No TMF8806!"));
    display.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_W, SCREEN_H);
    while (1)
      delay(10);
  }

  Serial.println(F("TMF8806 found!"));

  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.setIterations(400);
  tof.setRepetitionPeriod_ms(33);

  if (!tof.startMeasuring(true)) {
    Serial.println(F("Failed to start measurement!"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("Measuring..."));
}

void loop() {
  if (!tof.dataReady()) {
    delay(5);
    return;
  }

  tmf8806_result_t result;
  if (!tof.readResult(&result)) {
    delay(5);
    return;
  }

  uint16_t distance = result.distance;
  uint8_t reliability = result.reliability;
  uint16_t color = distanceColor(distance, reliability);

  if (distance > distMax) {
    distMax = distance + (distance / 4); // 25% headroom
  }

  canvas.fillScreen(ST77XX_BLACK);

  // ---- Title ----
  canvas.setFont(&FreeSansBold12pt7b);
  canvas.setTextColor(ST77XX_WHITE);
  int16_t x1, y1;
  uint16_t tw, th;
  canvas.getTextBounds("Adafruit TMF8806", 0, 0, &x1, &y1, &tw, &th);
  canvas.setCursor((SCREEN_W - tw) / 2, TITLE_Y);
  canvas.print(F("Adafruit TMF8806"));

  // ---- Distance bar ----
  uint16_t barX = BAR_MARGIN;
  uint16_t barW = SCREEN_W - (BAR_MARGIN * 2);
  canvas.drawRect(barX, BAR_Y, barW, BAR_H, ST77XX_WHITE);

  uint16_t fillW = (uint32_t)distance * (barW - 2) / distMax;
  if (fillW > 0) {
    canvas.fillRect(barX + 1, BAR_Y + 1, fillW, BAR_H - 2, color);
  }

  // "DIST" label centered in bar
  canvas.setFont(&FreeSansBold9pt7b);
  canvas.getTextBounds("DIST", 0, 0, &x1, &y1, &tw, &th);
  uint16_t labelX = (SCREEN_W - tw) / 2;
  uint16_t labelY = BAR_Y + BAR_H / 2 + th / 2;
  // Black outline for readability over fill
  for (int8_t dx = -1; dx <= 1; dx++) {
    for (int8_t dy = -1; dy <= 1; dy++) {
      if (dx == 0 && dy == 0)
        continue;
      canvas.setTextColor(ST77XX_BLACK);
      canvas.setCursor(labelX + dx, labelY + dy);
      canvas.print(F("DIST"));
    }
  }
  canvas.setTextColor(ST77XX_WHITE);
  canvas.setCursor(labelX, labelY);
  canvas.print(F("DIST"));

  // ---- Big distance readout ----
  char distBuf[8];
  snprintf(distBuf, sizeof(distBuf), "%u", distance);

  // Fixed position for "mm" label
  const int16_t distLabelX = 172;

  canvas.setFont(&FreeSans12pt7b);
  canvas.setTextColor(ST77XX_WHITE);
  canvas.setCursor(distLabelX, DIST_Y);
  canvas.print(F("mm"));

  // Right-align number just left of the label
  canvas.setFont(&FreeSansBold24pt7b);
  canvas.setTextColor(color);
  canvas.getTextBounds(distBuf, 0, 0, &x1, &y1, &tw, &th);
  canvas.setCursor(distLabelX - 8 - tw, DIST_Y);
  canvas.print(distBuf);

  display.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_W, SCREEN_H);
  delay(50);
}
