/*!
 * @file tmf8806_lowpower.ino
 *
 * Low-power example for TMF8806 Time-of-Flight Distance Sensor.
 * Takes a single-shot distance reading, then sleeps the sensor for
 * 10 seconds before waking and reading again. The TMF8806 draws ~3uA
 * in sleep mode vs ~2mA active.
 *
 * Limor 'ladyada' Fried with assistance from Claude Code
 * MIT License
 */

#include <Adafruit_TMF8806.h>

Adafruit_TMF8806 tof;

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
}

void loop() {
  // Wake the sensor from sleep
  if (!tof.wakeup()) {
    Serial.println(F("Failed to wake sensor!"));
    delay(10000);
    return;
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

  // Put sensor to sleep (~3uA)
  tof.sleep();
  Serial.println(F("Sleeping 10 seconds..."));
  Serial.println();

  delay(10000);
}
