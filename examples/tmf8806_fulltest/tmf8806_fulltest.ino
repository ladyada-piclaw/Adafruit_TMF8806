/*!
 * @file tmf8806_fulltest.ino
 *
 * Full feature demo for the Adafruit TMF8806 Time-of-Flight sensor.
 * Cycles through all available configurations so you can see how each
 * setting affects distance readings, reliability, and measurement speed.
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * BSD license, all text here must be included in any redistribution.
 */

#include <Adafruit_TMF8806.h>

Adafruit_TMF8806 tof;

// Helper: take N readings and print stats
void measureAndReport(uint8_t numReadings, uint16_t timeoutMs) {
  uint16_t minDist = 65535, maxDist = 0;
  uint32_t sumDist = 0;
  uint8_t minRel = 63, maxRel = 0;
  uint8_t count = 0;
  uint32_t t0 = millis();

  for (uint8_t i = 0; i < numReadings; i++) {
    uint32_t start = millis();
    while ((millis() - start) < timeoutMs) {
      if (tof.dataReady()) {
        tmf8806_result_t r;
        if (tof.readResult(&r) && r.reliability > 0) {
          if (r.distance < minDist) minDist = r.distance;
          if (r.distance > maxDist) maxDist = r.distance;
          if (r.reliability < minRel) minRel = r.reliability;
          if (r.reliability > maxRel) maxRel = r.reliability;
          sumDist += r.distance;
          count++;
        }
        break;
      }
      delay(1);
    }
  }

  uint32_t elapsed = millis() - t0;

  if (count > 0) {
    Serial.print(F("  Avg dist: "));
    Serial.print(sumDist / count);
    Serial.print(F(" mm  ("));
    Serial.print(minDist);
    Serial.print(F("-"));
    Serial.print(maxDist);
    Serial.println(F(" mm)"));
    Serial.print(F("  Reliability: "));
    Serial.print(minRel);
    Serial.print(F("-"));
    Serial.println(maxRel);
  } else {
    Serial.println(F("  No valid readings (no object detected?)"));
  }
  Serial.print(F("  "));
  Serial.print(count);
  Serial.print(F("/"));
  Serial.print(numReadings);
  Serial.print(F(" readings in "));
  Serial.print(elapsed);
  Serial.println(F(" ms"));
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println(F("========================================"));
  Serial.println(F(" TMF8806 Full Feature Demo"));
  Serial.println(F("========================================"));
  Serial.println();

  if (!tof.begin()) {
    Serial.println(F("Failed to find TMF8806 sensor!"));
    while (1) delay(10);
  }

  // --- Device info ---
  Serial.println(F("--- Device Info ---"));
  Serial.print(F("Chip ID: 0x"));
  Serial.println(tof.getChipID(), HEX);
  Serial.print(F("Revision: "));
  Serial.println(tof.getRevisionID());

  uint8_t major, minor, patch;
  tof.getVersion(&major, &minor, &patch);
  Serial.print(F("Firmware: "));
  Serial.print(major);
  Serial.print(F("."));
  Serial.print(minor);
  Serial.print(F("."));
  Serial.println(patch);

  uint8_t serial[4];
  if (tof.readSerialNumber(serial, 4)) {
    Serial.print(F("Serial: 0x"));
    for (int i = 3; i >= 0; i--) {
      if (serial[i] < 0x10) Serial.print(F("0"));
      Serial.print(serial[i], HEX);
    }
    Serial.println();
  }
  Serial.println();

  // =============================================
  // Distance Modes
  // =============================================
  Serial.println(F("=== Distance Modes ==="));
  Serial.println(F("(10 readings each, default 400k iterations)"));
  Serial.println();

  // Short range
  Serial.println(F(">> Short Range (max 200mm)"));
  tof.setDistanceMode(TMF8806_MODE_SHORT_RANGE);
  tof.setIterations(400);
  tof.setRepetitionPeriod(33);
  tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_97NS);
  tof.setOpticalConfig(TMF8806_SPAD_DEFAULT);
  tof.startMeasuring(true);
  measureAndReport(10, 200);
  tof.stopMeasuring();
  delay(50);

  // 2.5m mode
  Serial.println(F(">> 2.5m Mode (max 2650mm)"));
  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.startMeasuring(true);
  measureAndReport(10, 200);
  tof.stopMeasuring();
  delay(50);

  // 5m mode
  Serial.println(F(">> 5m Mode (max 5300mm)"));
  tof.setDistanceMode(TMF8806_MODE_5M);
  tof.setRepetitionPeriod(66);
  tof.startMeasuring(true);
  measureAndReport(10, 300);
  tof.stopMeasuring();
  delay(50);

  Serial.println();

  // =============================================
  // Iterations (affects accuracy vs speed)
  // =============================================
  Serial.println(F("=== Iterations (accuracy vs speed) ==="));
  Serial.println(F("(10 readings each, 2.5m mode)"));
  Serial.println();

  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_97NS);

  uint16_t iters[] = {50, 100, 200, 400, 900};
  for (uint8_t i = 0; i < 5; i++) {
    Serial.print(F(">> "));
    Serial.print(iters[i]);
    Serial.println(F("k iterations"));
    tof.setIterations(iters[i]);
    tof.setRepetitionPeriod(10); // fast repetition to show speed diff
    tof.startMeasuring(true);
    measureAndReport(10, 500);
    tof.stopMeasuring();
    delay(50);
  }

  Serial.println();

  // =============================================
  // SPAD Dead Time (short-range accuracy vs sunlight)
  // =============================================
  Serial.println(F("=== SPAD Dead Time ==="));
  Serial.println(F("(10 readings each, 2.5m mode, 400k iters)"));
  Serial.println();

  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.setIterations(400);
  tof.setRepetitionPeriod(33);

  for (uint8_t i = 0; i < 8; i++) {
    Serial.print(F(">> SPAD dead time "));
    Serial.print(i);
    Serial.print(F(": "));
    switch (i) {
      case 0: Serial.println(F("97ns (best short-range)")); break;
      case 1: Serial.println(F("48ns")); break;
      case 2: Serial.println(F("32ns")); break;
      case 3: Serial.println(F("24ns")); break;
      case 4: Serial.println(F("16ns (default balance)")); break;
      case 5: Serial.println(F("12ns")); break;
      case 6: Serial.println(F("8ns")); break;
      case 7: Serial.println(F("4ns (best sunlight)")); break;
    }
    tof.setSpadDeadTime((tmf8806_spad_deadtime_t)i);
    tof.startMeasuring(true);
    measureAndReport(10, 200);
    tof.stopMeasuring();
    delay(50);
  }

  Serial.println();

  // =============================================
  // Optical Configurations
  // =============================================
  Serial.println(F("=== Optical Configurations ==="));
  Serial.println(F("(10 readings each, 2.5m mode)"));
  Serial.println();

  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.setIterations(400);
  tof.setRepetitionPeriod(33);
  tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_97NS);

  Serial.println(F(">> Default (0.5mm airgap, 0.55mm glass)"));
  tof.setOpticalConfig(TMF8806_SPAD_DEFAULT);
  tof.startMeasuring(true);
  measureAndReport(10, 200);
  tof.stopMeasuring();
  delay(50);

  Serial.println(F(">> Large Airgap (1mm, min 20mm)"));
  tof.setOpticalConfig(TMF8806_SPAD_LARGE_AIRGAP);
  tof.startMeasuring(true);
  measureAndReport(10, 200);
  tof.stopMeasuring();
  delay(50);

  Serial.println(F(">> Thick Glass (3.2mm, min 40mm)"));
  tof.setOpticalConfig(TMF8806_SPAD_THICK_GLASS);
  tof.startMeasuring(true);
  measureAndReport(10, 200);
  tof.stopMeasuring();
  delay(50);

  Serial.println();

  // =============================================
  // Repetition Period
  // =============================================
  Serial.println(F("=== Repetition Period ==="));
  Serial.println(F("(10 readings each, 2.5m mode, 400k iters)"));
  Serial.println();

  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.setIterations(400);
  tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_97NS);
  tof.setOpticalConfig(TMF8806_SPAD_DEFAULT);

  uint8_t periods[] = {10, 33, 66, 100, 250};
  for (uint8_t i = 0; i < 5; i++) {
    Serial.print(F(">> "));
    Serial.print(periods[i]);
    Serial.println(F(" ms period"));
    tof.setRepetitionPeriod(periods[i]);
    tof.startMeasuring(true);
    measureAndReport(10, 500);
    tof.stopMeasuring();
    delay(50);
  }

  Serial.println();

  // =============================================
  // Single-shot mode
  // =============================================
  Serial.println(F("=== Single-Shot Mode ==="));
  Serial.println();

  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.setIterations(400);
  tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_97NS);

  Serial.println(F(">> Taking 3 single-shot readings..."));
  for (uint8_t i = 0; i < 3; i++) {
    tof.startMeasuring(false); // single-shot
    uint32_t t0 = millis();
    while (!tof.dataReady() && (millis() - t0) < 500) delay(1);
    tmf8806_result_t r;
    if (tof.readResult(&r) && r.reliability > 0) {
      Serial.print(F("  Shot "));
      Serial.print(i + 1);
      Serial.print(F(": "));
      Serial.print(r.distance);
      Serial.print(F(" mm, rel="));
      Serial.print(r.reliability);
      Serial.print(F(", "));
      Serial.print(millis() - t0);
      Serial.println(F(" ms"));
    } else {
      Serial.print(F("  Shot "));
      Serial.print(i + 1);
      Serial.println(F(": no object"));
    }
    delay(50);
  }

  Serial.println();

  // =============================================
  // Factory Calibration
  // =============================================
  Serial.println(F("=== Factory Calibration ==="));
  Serial.println(F("(Ensure clear FoV >40cm, low ambient light)"));
  Serial.println();

  Serial.println(F(">> Running calibration..."));
  if (tof.performFactoryCalibration()) {
    uint8_t calData[14];
    if (tof.getCalibrationData(calData, 14)) {
      Serial.print(F("  Cal data: "));
      for (uint8_t i = 0; i < 14; i++) {
        if (calData[i] < 0x10) Serial.print(F("0"));
        Serial.print(calData[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();
    }

    // Compare calibrated vs uncalibrated
    Serial.println(F(">> Uncalibrated:"));
    tof.enableCalibration(false);
    tof.setDistanceMode(TMF8806_MODE_2_5M);
    tof.setIterations(400);
    tof.setRepetitionPeriod(33);
    tof.startMeasuring(true);
    measureAndReport(10, 200);
    tof.stopMeasuring();
    delay(50);

    Serial.println(F(">> Calibrated:"));
    tof.enableCalibration(true);
    tof.startMeasuring(true);
    measureAndReport(10, 200);
    tof.stopMeasuring();
  } else {
    Serial.println(F("  Calibration failed!"));
  }

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" Full test complete!"));
  Serial.println(F("========================================"));
}

void loop() { delay(1000); }
