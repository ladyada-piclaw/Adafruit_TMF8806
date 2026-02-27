/*!
 * @file tmf8806_lowpower.ino
 *
 * Low-power example for TMF8806 Time-of-Flight Distance Sensor.
 * Takes a single-shot distance reading, saves algorithm state, then
 * sleeps the sensor for 10 seconds before waking and reading again.
 * The TMF8806 draws ~3uA in sleep mode vs ~2mA active.
 *
 * Algorithm state is saved between sleep cycles so the sensor
 * converges faster on wakeup (better accuracy on first reading).
 *
 * Limor 'ladyada' Fried with assistance from Claude Code
 * MIT License
 */

#include <Adafruit_TMF8806.h>

Adafruit_TMF8806 tof;

uint8_t calData[TMF8806_CALIB_DATA_SIZE];
uint8_t stateData[TMF8806_STATE_DATA_SIZE];
bool hasState = false;

void setup() {
  Serial.begin(115200);

  while (!Serial)
    delay(10);

  Serial.println(F("TMF8806 Low Power Example"));

  if (!tof.begin()) {
    Serial.println(F("Failed to find TMF8806 chip"));
    while (1)
      delay(10);
  }

  Serial.println(F("TMF8806 found!"));

  // Use single-shot mode (repetition period = 0)
  tof.setRepetitionPeriod_ms(0);
  tof.setDistanceMode(TMF8806_MODE_2_5M);
  tof.setIterations(400);

  // Run factory calibration once at startup
  Serial.println(F("Running factory calibration..."));
  if (tof.performFactoryCalibration()) {
    tof.getCalibrationData(calData);
    tof.setCalibrationData(calData);
    tof.enableCalibration(true);
    Serial.println(F("Calibration done!"));
  } else {
    Serial.println(F("Calibration failed, continuing without it"));
  }

  Serial.println();
}

void loop() {
  // Wake the sensor from sleep
  if (!tof.wakeup()) {
    Serial.println(F("Failed to wake sensor!"));
    delay(10000);
    return;
  }

  // Restore algorithm state from previous cycle (if available)
  if (hasState) {
    tof.setAlgorithmState(stateData);
    tof.enableAlgorithmState(true);
  }

  // Take a single-shot measurement
  if (!tof.startMeasuring(false)) {
    Serial.println(F("Failed to start measurement!"));
    tof.sleep();
    delay(10000);
    return;
  }

  // Wait for result
  uint32_t startMs = millis();
  while (!tof.dataReady()) {
    if (millis() - startMs > 1000) {
      Serial.println(F("Measurement timeout!"));
      tof.sleep();
      delay(10000);
      return;
    }
    delay(1);
  }

  // Read and print result
  tmf8806_result_t result;
  if (tof.readResult(&result) && result.reliability > 0) {
    Serial.print(F("Distance: "));
    Serial.print(result.distance);
    Serial.print(F(" mm  Reliability: "));
    Serial.print(result.reliability);
    Serial.println(F("/63"));
  } else {
    Serial.println(F("No object detected"));
  }

  // Save algorithm state for next cycle
  if (tof.getAlgorithmState(stateData)) {
    hasState = true;
  }

  // Put sensor to sleep (~3uA)
  tof.sleep();
  Serial.println(F("Sleeping 10 seconds..."));
  Serial.println();

  delay(10000);
}
