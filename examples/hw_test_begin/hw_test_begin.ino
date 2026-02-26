/*!
 * @file hw_test_begin.ino
 * HW Test: begin(), chipID, version, serial number, reset
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

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println(F("=== HW TEST: begin ==="));
  Serial.println();

  // Test begin()
  bool ok = tof.begin();
  check(F("begin()"), ok);
  if (!ok) {
    Serial.println(F("ABORT: sensor not found"));
    while (1) delay(10);
  }

  // Test chip ID
  uint8_t id = tof.getChipID();
  Serial.print(F("  Chip ID: 0x"));
  Serial.println(id, HEX);
  check(F("chipID == 0x09"), id == 0x09);

  // Test firmware version
  uint8_t major, minor, patch;
  tof.getVersion(&major, &minor, &patch);
  Serial.print(F("  FW: "));
  Serial.print(major); Serial.print(F("."));
  Serial.print(minor); Serial.print(F("."));
  Serial.println(patch);
  check(F("FW major > 0"), major > 0);

  // Test serial number
  uint8_t serial[4] = {0};
  ok = tof.readSerialNumber(serial, 4);
  Serial.print(F("  Serial: 0x"));
  for (int i = 3; i >= 0; i--) {
    if (serial[i] < 0x10) Serial.print(F("0"));
    Serial.print(serial[i], HEX);
  }
  Serial.println();
  check(F("readSerialNumber()"), ok);
  // Serial should be nonzero
  uint32_t sn = ((uint32_t)serial[3] << 24) | ((uint32_t)serial[2] << 16) |
                ((uint32_t)serial[1] << 8) | serial[0];
  check(F("serial != 0"), sn != 0);

  // Test reset and re-begin
  ok = tof.reset();
  check(F("reset()"), ok);

  // After reset, chip ID should still work
  id = tof.getChipID();
  check(F("chipID after reset"), id == 0x09);

  Serial.println();
  Serial.println(F("=== RESULTS ==="));
  Serial.print(passes); Serial.print(F(" passed, "));
  Serial.print(fails); Serial.println(F(" failed"));
  Serial.println(fails == 0 ? F("ALL PASS") : F("SOME FAILED"));
}

void loop() { delay(1000); }
