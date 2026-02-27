/*!
 * @file hw_test_pins.ino
 * HW Test: INT, EN, GPIO0, GPIO1 pin behavior
 *
 * Wiring:
 *   TMF8806 INT  -> Metro D2
 *   TMF8806 EN   -> Metro D3
 *   TMF8806 GPIO0 -> Metro D5
 *   TMF8806 GPIO1 -> Metro D6
 *   Servo on D4 (not used in this test)
 */

#include <Adafruit_TMF8806.h>

#define PIN_INT 2
#define PIN_EN 3
#define PIN_GPIO0 5
#define PIN_GPIO1 6

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
  Serial.println(F("=== HW TEST: pins ==="));
  Serial.println();

  pinMode(PIN_INT, INPUT_PULLUP);
  pinMode(PIN_EN, OUTPUT);
  pinMode(PIN_GPIO0, INPUT);
  pinMode(PIN_GPIO1, INPUT);

  // Start with EN HIGH
  digitalWrite(PIN_EN, HIGH);
  delay(10);

  if (!tof.begin()) {
    Serial.println(F("ABORT: sensor not found"));
    while (1)
      delay(10);
  }

  // ==========================================================
  // TEST: INT pin
  // ==========================================================
  Serial.println(F("-- INT pin --"));

  // INT should be HIGH (inactive) when no data pending
  // Clear any pending interrupts first
  if (tof.dataReady()) {
    tmf8806_result_t dummy;
    tof.readResult(&dummy);
  }
  delay(10);
  check(F("INT HIGH when idle"), digitalRead(PIN_INT) == HIGH);

  // Start measurement, wait for data, INT should go LOW
  tof.startMeasuring(true);
  delay(100); // Wait for at least one measurement

  // Wait for INT to assert
  uint32_t t0 = millis();
  while (digitalRead(PIN_INT) == HIGH && (millis() - t0) < 500) {
    delay(1);
  }
  check(F("INT LOW when data ready"), digitalRead(PIN_INT) == LOW);

  // Verify dataReady agrees
  check(F("dataReady matches INT"), tof.dataReady());

  // Read result should clear interrupt and INT goes HIGH
  tmf8806_result_t result;
  tof.readResult(&result);
  delay(1);
  // INT may reassert quickly in continuous mode, so check immediately
  // Instead verify we got valid data when INT was low
  check(F("got data when INT low"), result.distance > 0);

  tof.stopMeasuring();
  // After stop + clear, INT should be HIGH
  if (tof.dataReady()) {
    tof.readResult(&result);
  }
  delay(50);
  check(F("INT HIGH after stop"), digitalRead(PIN_INT) == HIGH);

  // ==========================================================
  // TEST: EN pin (power cycling)
  // ==========================================================
  Serial.println();
  Serial.println(F("-- EN pin --"));

  // Disable sensor via EN
  digitalWrite(PIN_EN, LOW);
  delay(10);

  // Sensor should not respond on I2C
  // Try to read chip ID - should fail or return wrong value
  Wire.beginTransmission(0x41);
  uint8_t i2cErr = Wire.endTransmission();
  check(F("sensor gone when EN LOW"), i2cErr != 0);

  // Re-enable
  digitalWrite(PIN_EN, HIGH);
  delay(10);

  // Re-initialize
  bool ok = tof.begin();
  check(F("begin() after EN toggle"), ok);

  // Verify sensor is working
  uint8_t id = tof.getChipID();
  check(F("chipID after EN toggle"), id == 0x09);

  // Start measuring to verify full functionality restored
  ok = tof.startMeasuring(true);
  check(F("measure after EN toggle"), ok);
  if (waitForData(200)) {
    tof.readResult(&result);
    check(F("valid data after EN toggle"), result.distance > 0);
  } else {
    check(F("valid data after EN toggle"), false);
  }
  tof.stopMeasuring();
  delay(50);

  // ==========================================================
  // TEST: GPIO0 output modes
  // NOTE: GPIO0 is used for I/O voltage detection at startup,
  // which may interfere with output LOW behavior.
  // ==========================================================
  Serial.println();
  Serial.println(F("-- GPIO0 output --"));

  // GPIO config is applied via startMeasuring()
  // Set GPIO0 to output LOW
  tof.setGPIOMode(0, TMF8806_GPIO_OUTPUT_LOW);
  tof.startMeasuring(true);
  delay(50);
  int g0low = digitalRead(PIN_GPIO0);
  Serial.print(F("  GPIO0 set LOW, read="));
  Serial.println(g0low);
  check(F("GPIO0 output LOW"), g0low == LOW);
  tof.stopMeasuring();
  delay(50);

  // Set GPIO0 to output HIGH
  tof.setGPIOMode(0, TMF8806_GPIO_OUTPUT_HIGH);
  tof.startMeasuring(true);
  delay(50);
  int g0high = digitalRead(PIN_GPIO0);
  Serial.print(F("  GPIO0 set HIGH, read="));
  Serial.println(g0high);
  check(F("GPIO0 output HIGH"), g0high == HIGH);
  tof.stopMeasuring();
  delay(50);

  // ==========================================================
  // TEST: GPIO1 output modes
  // ==========================================================
  Serial.println();
  Serial.println(F("-- GPIO1 output --"));

  // Set GPIO1 to output LOW, GPIO0 back to disabled
  tof.setGPIOMode(0, TMF8806_GPIO_DISABLED);
  tof.setGPIOMode(1, TMF8806_GPIO_OUTPUT_LOW);
  tof.startMeasuring(true);
  delay(50);
  int g1low = digitalRead(PIN_GPIO1);
  Serial.print(F("  GPIO1 set LOW, read="));
  Serial.println(g1low);
  check(F("GPIO1 output LOW"), g1low == LOW);
  tof.stopMeasuring();
  delay(50);

  // Set GPIO1 to output HIGH
  tof.setGPIOMode(1, TMF8806_GPIO_OUTPUT_HIGH);
  tof.startMeasuring(true);
  delay(50);
  int g1high = digitalRead(PIN_GPIO1);
  Serial.print(F("  GPIO1 set HIGH, read="));
  Serial.println(g1high);
  check(F("GPIO1 output HIGH"), g1high == HIGH);
  tof.stopMeasuring();
  delay(50);

  // Reset GPIOs to disabled
  tof.setGPIOMode(0, TMF8806_GPIO_DISABLED);
  tof.setGPIOMode(1, TMF8806_GPIO_DISABLED);

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
