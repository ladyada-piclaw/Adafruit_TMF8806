/*!
 * @file hw_test_config.ino
 * HW Test: distance modes, iterations, repetition period, threshold, SPAD
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

uint16_t measureOnce(uint16_t timeoutMs) {
  if (waitForData(timeoutMs)) {
    tmf8806_result_t r;
    if (tof.readResult(&r))
      return r.distance;
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  Serial.println(F("=== HW TEST: config ==="));
  Serial.println();

  if (!tof.begin()) {
    Serial.println(F("ABORT: sensor not found"));
    while (1)
      delay(10);
  }

  // --- Test 1: 2.5m mode (default) ---
  Serial.println(F("-- 2.5m mode --"));
  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.setIterations(400);
  tof.setRepetitionPeriod_ms(33);
  bool ok = tof.startMeasuring(true);
  check(F("start 2.5m"), ok);
  uint16_t dist25 = measureOnce(200);
  Serial.print(F("  dist="));
  Serial.println(dist25);
  check(F("2.5m reading valid"), dist25 > 0 && dist25 < 2650);
  tof.stopMeasuring();
  delay(50);

  // --- Test 2: 5m mode ---
  Serial.println(F("-- 5m mode --"));
  tof.setDistanceMode(TMF8806_MODE_5M);
  tof.setIterations(400);
  tof.setRepetitionPeriod_ms(66);
  ok = tof.startMeasuring(true);
  check(F("start 5m"), ok);
  uint16_t dist5 = measureOnce(300);
  Serial.print(F("  dist="));
  Serial.println(dist5);
  check(F("5m reading valid"), dist5 > 0 && dist5 < 5300);
  // Distance should be roughly the same as 2.5m mode
  int16_t diff = (int16_t)dist5 - (int16_t)dist25;
  if (diff < 0)
    diff = -diff;
  Serial.print(F("  diff from 2.5m="));
  Serial.println(diff);
  check(F("5m agrees with 2.5m (<100mm)"), diff < 100);
  tof.stopMeasuring();
  delay(50);

  // --- Test 3: Short range mode ---
  Serial.println(F("-- Short range mode --"));
  tof.setDistanceMode(TMF8806_MODE_SHORT_RANGE);
  tof.setIterations(400);
  tof.setRepetitionPeriod_ms(33);
  ok = tof.startMeasuring(true);
  check(F("start short range"), ok);
  // Short range max is 200mm - if target is far, distance should be 0 or
  // clamped
  uint16_t distShort = measureOnce(200);
  Serial.print(F("  dist="));
  Serial.println(distShort);
  // We expect 0 (clamped) since target is likely > 200mm away
  check(F("short range dist <= 200"), distShort <= 200);
  tof.stopMeasuring();
  delay(50);

  // --- Test 4: Different iterations ---
  Serial.println(F("-- Iterations test --"));
  tof.setDistanceMode(TMF8806_MODE_2_5M);

  // Low iterations (faster, less accurate)
  tof.setIterations(100);
  tof.setRepetitionPeriod_ms(10);
  ok = tof.startMeasuring(true);
  check(F("start 100k iters"), ok);
  uint32_t t0 = millis();
  uint16_t distLow = measureOnce(200);
  uint32_t tLow = millis() - t0;
  Serial.print(F("  100k: dist="));
  Serial.print(distLow);
  Serial.print(F(" time="));
  Serial.print(tLow);
  Serial.println(F("ms"));
  check(F("100k iters got reading"), distLow > 0);
  tof.stopMeasuring();
  delay(50);

  // High iterations (slower, more accurate)
  tof.setIterations(900);
  tof.setRepetitionPeriod_ms(100);
  ok = tof.startMeasuring(true);
  check(F("start 900k iters"), ok);
  t0 = millis();
  uint16_t distHigh = measureOnce(500);
  uint32_t tHigh = millis() - t0;
  Serial.print(F("  900k: dist="));
  Serial.print(distHigh);
  Serial.print(F(" time="));
  Serial.print(tHigh);
  Serial.println(F("ms"));
  check(F("900k iters got reading"), distHigh > 0);
  check(F("900k slower than 100k"), tHigh > tLow);
  tof.stopMeasuring();
  delay(50);

  // --- Test 5: SPAD dead time ---
  Serial.println(F("-- SPAD dead time --"));
  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.setIterations(400);
  tof.setRepetitionPeriod_ms(33);

  tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_97NS);
  ok = tof.startMeasuring(true);
  check(F("start deadtime 97ns"), ok);
  uint16_t dist97 = measureOnce(200);
  Serial.print(F("  97ns: dist="));
  Serial.println(dist97);
  check(F("97ns reading valid"), dist97 > 0);
  tof.stopMeasuring();
  delay(50);

  tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_4NS);
  ok = tof.startMeasuring(true);
  check(F("start deadtime 4ns"), ok);
  uint16_t dist4 = measureOnce(200);
  Serial.print(F("  4ns: dist="));
  Serial.println(dist4);
  check(F("4ns reading valid"), dist4 > 0);
  tof.stopMeasuring();
  delay(50);

  // --- Test 6: Optical config ---
  Serial.println(F("-- Optical config --"));
  tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_16NS); // reset
  tof.setOpticalConfig(TMF8806_SPAD_DEFAULT);
  ok = tof.startMeasuring(true);
  check(F("start SPAD default"), ok);
  uint16_t distDef = measureOnce(200);
  Serial.print(F("  default: dist="));
  Serial.println(distDef);
  check(F("SPAD default reading"), distDef > 0);
  tof.stopMeasuring();
  delay(50);

  tof.setOpticalConfig(TMF8806_SPAD_LARGE_AIRGAP);
  ok = tof.startMeasuring(true);
  check(F("start SPAD large airgap"), ok);
  uint16_t distAir = measureOnce(200);
  Serial.print(F("  large airgap: dist="));
  Serial.println(distAir);
  check(F("SPAD airgap reading"), distAir > 0);
  tof.stopMeasuring();

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
