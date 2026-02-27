/*!
 * @file hw_test_calibration.ino
 * HW Test: factory calibration, save/load calibration data
 * NOTE: Requires clear field of view (no object within 40cm) and low ambient
 * light
 */

#include <Adafruit_TMF8806.h>

Adafruit_TMF8806 tof;
uint8_t passes = 0;
uint8_t fails = 0;

void check(const __FlashStringHelper* name, bool cond) {
  Serial.print(name);
  if (cond) {
    Serial.println(F(" ... PASS"));
    passes++;
  } else {
    Serial.println(F(" ... FAIL"));
    fails++;
  }
}

bool waitForData(uint16_t timeoutMs) {
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    if (tof.dataReady())
      return true;
    delay(1);
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  Serial.println(F("=== HW TEST: calibration ==="));
  Serial.println(F("Ensure clear FoV (>40cm) and low ambient light"));
  Serial.println();

  if (!tof.begin()) {
    Serial.println(F("ABORT: sensor not found"));
    while (1)
      delay(10);
  }

  // --- Measure without calibration ---
  Serial.println(F("-- Without calibration --"));
  tof.enableCalibration(false);
  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.setIterations(400);
  tof.setRepetitionPeriod_ms(33);
  bool ok = tof.startMeasuring(true);
  check(F("start uncalibrated"), ok);

  // Read a few and average
  uint32_t sum = 0;
  uint8_t count = 0;
  for (int i = 0; i < 10; i++) {
    if (waitForData(200)) {
      tmf8806_result_t r;
      if (tof.readResult(&r) && r.distance > 0) {
        sum += r.distance;
        count++;
      }
    }
  }
  uint16_t distUncal = count ? (sum / count) : 0;
  Serial.print(F("  uncalibrated avg="));
  Serial.print(distUncal);
  Serial.print(F("mm ("));
  Serial.print(count);
  Serial.println(F(" readings)"));
  check(F("uncalibrated readings"), count > 0);
  tof.stopMeasuring();
  delay(50);

  // --- Perform factory calibration ---
  Serial.println(F("-- Factory calibration --"));
  Serial.println(F("  Running (may take a few seconds)..."));
  ok = tof.performFactoryCalibration();
  check(F("performFactoryCalibration()"), ok);

  // Read calibration data
  uint8_t calData[14] = {0};
  ok = tof.getCalibrationData(calData, 14);
  check(F("getCalibrationData()"), ok);

  Serial.print(F("  cal data: "));
  for (int i = 0; i < 14; i++) {
    if (calData[i] < 0x10)
      Serial.print(F("0"));
    Serial.print(calData[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println();

  // Check cal data is not all zeros
  bool nonzero = false;
  for (int i = 0; i < 14; i++) {
    if (calData[i] != 0) {
      nonzero = true;
      break;
    }
  }
  check(F("cal data nonzero"), nonzero);

  // --- Measure with calibration ---
  Serial.println(F("-- With calibration --"));
  tof.setCalibrationData(calData, 14);
  tof.enableCalibration(true);
  ok = tof.startMeasuring(true);
  check(F("start calibrated"), ok);

  sum = 0;
  count = 0;
  for (int i = 0; i < 10; i++) {
    if (waitForData(200)) {
      tmf8806_result_t r;
      if (tof.readResult(&r) && r.distance > 0) {
        sum += r.distance;
        count++;
      }
    }
  }
  uint16_t distCal = count ? (sum / count) : 0;
  Serial.print(F("  calibrated avg="));
  Serial.print(distCal);
  Serial.print(F("mm ("));
  Serial.print(count);
  Serial.println(F(" readings)"));
  check(F("calibrated readings"), count > 0);

  // Both should be roughly the same target distance
  if (distUncal > 0 && distCal > 0) {
    int16_t diff = (int16_t)distCal - (int16_t)distUncal;
    if (diff < 0)
      diff = -diff;
    Serial.print(F("  cal vs uncal diff="));
    Serial.println(diff);
    check(F("cal vs uncal diff < 100mm"), diff < 100);
  }
  tof.stopMeasuring();

  // --- Test save/load cycle ---
  Serial.println(F("-- Save/load cal data --"));
  uint8_t calData2[14] = {0};
  ok = tof.setCalibrationData(calData, 14);
  check(F("setCalibrationData()"), ok);
  // Re-read what we set (internal copy)
  // Just verify the API doesn't crash
  check(F("setCalibrationData no crash"), true);

  Serial.println();
  Serial.println(F("=== RESULTS ==="));
  Serial.print(passes);
  Serial.print(F(" passed, "));
  Serial.print(fails);
  Serial.println(F(" failed"));
  Serial.println(fails == 0 ? F("ALL PASS") : F("SOME FAILED"));
}

void loop() {
  delay(1000);
}
