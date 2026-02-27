/*!
 * @file hw_test_measure.ino
 * HW Test: continuous vs single-shot, readDistance(), readResult(), dataReady()
 */

#include <Adafruit_TMF8806.h>

Adafruit_TMF8806 tof;
uint8_t passes = 0;
uint8_t fails = 0;

void check(const __FlashStringHelper *name, bool cond) {
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
    if (tof.dataReady()) return true;
    delay(1);
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println(F("=== HW TEST: measure ==="));
  Serial.println();

  if (!tof.begin()) {
    Serial.println(F("ABORT: sensor not found"));
    while (1) delay(10);
  }

  // --- Test 1: Continuous measurement ---
  Serial.println(F("-- Continuous mode --"));
  bool ok = tof.startMeasuring(true);
  check(F("startMeasuring(continuous)"), ok);

  // Should get data within 100ms at default 33ms period
  ok = waitForData(200);
  check(F("dataReady within 200ms"), ok);

  if (ok) {
    tmf8806_result_t result;
    ok = tof.readResult(&result);
    check(F("readResult()"), ok);
    Serial.print(F("  dist="));
    Serial.print(result.distance);
    Serial.print(F(" rel="));
    Serial.print(result.reliability);
    Serial.print(F(" stat="));
    Serial.print(result.status);
    Serial.print(F(" temp="));
    Serial.println(result.temperature);
    check(F("distance > 0"), result.distance > 0);
    check(F("distance < 5300"), result.distance < 5300);
    check(F("reliability > 0"), result.reliability > 0);
  }

  // Read 5 more samples, check consistency
  uint16_t minDist = 65535, maxDist = 0;
  for (int i = 0; i < 5; i++) {
    if (waitForData(200)) {
      tmf8806_result_t r;
      if (tof.readResult(&r)) {
        if (r.distance < minDist) minDist = r.distance;
        if (r.distance > maxDist) maxDist = r.distance;
      }
    }
  }
  Serial.print(F("  range: "));
  Serial.print(minDist);
  Serial.print(F("-"));
  Serial.print(maxDist);
  Serial.println(F(" mm"));
  check(F("5 reads spread < 50mm"), (maxDist - minDist) < 50);

  // Stop
  ok = tof.stopMeasuring();
  check(F("stopMeasuring()"), ok);

  // After stop, no new data within 200ms
  delay(100);
  // Clear any pending
  if (tof.dataReady()) {
    tmf8806_result_t dummy;
    tof.readResult(&dummy);
  }
  ok = !waitForData(200);
  check(F("no data after stop"), ok);

  // --- Test 2: readDistance() convenience ---
  Serial.println();
  Serial.println(F("-- readDistance() --"));
  ok = tof.startMeasuring(true);
  check(F("startMeasuring again"), ok);
  delay(100);

  int16_t dist = tof.readDistance();
  Serial.print(F("  readDistance()="));
  Serial.println(dist);
  check(F("readDistance > 0"), dist > 0);

  tof.stopMeasuring();

  // --- Test 3: Single-shot ---
  Serial.println();
  Serial.println(F("-- Single-shot mode --"));
  ok = tof.startMeasuring(false);
  check(F("startMeasuring(single)"), ok);

  ok = waitForData(500);
  check(F("single-shot data ready"), ok);

  if (ok) {
    tmf8806_result_t result;
    ok = tof.readResult(&result);
    check(F("single-shot readResult"), ok);
    Serial.print(F("  dist="));
    Serial.println(result.distance);
    check(F("single distance > 0"), result.distance > 0);
  }

  // --- Test 4: Temperature ---
  Serial.println();
  int8_t temp = tof.getTemperature();
  Serial.print(F("  getTemperature()="));
  Serial.println(temp);
  check(F("temp 10-50C"), temp >= 10 && temp <= 50);

  Serial.println();
  Serial.println(F("=== RESULTS ==="));
  Serial.print(passes); Serial.print(F(" passed, "));
  Serial.print(fails); Serial.println(F(" failed"));
  Serial.println(fails == 0 ? F("ALL PASS") : F("SOME FAILED"));
}

void loop() { delay(1000); }
