/*!
 * @file tmf8806_simpletest.ino
 *
 * Simple example for the Adafruit TMF8806 Time-of-Flight sensor.
 * Reads distance continuously and prints results to Serial.
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * BSD license, all text here must be included in any redistribution.
 */

#include <Adafruit_TMF8806.h>

Adafruit_TMF8806 tof;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8806 Time-of-Flight Sensor Test"));

  if (!tof.begin()) {
    Serial.println(F("Failed to find TMF8806 sensor!"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("TMF8806 found!"));

  // Print chip ID and revision
  uint8_t chipId = tof.getChipID();
  Serial.print(F("Chip ID: 0x"));
  Serial.println(chipId, HEX);
  Serial.print(F("Revision ID: "));
  Serial.println(tof.getRevisionID());

  // Print firmware version
  uint8_t major, minor, patch;
  tof.getVersion(&major, &minor, &patch);
  Serial.print(F("Firmware version: "));
  Serial.print(major);
  Serial.print(F("."));
  Serial.print(minor);
  Serial.print(F("."));
  Serial.println(patch);

  // Read and print serial number
  uint8_t serial[4];
  if (tof.readSerialNumber(serial, 4)) {
    Serial.print(F("Serial number: 0x"));
    for (int i = 3; i >= 0; i--) {
      if (serial[i] < 0x10) {
        Serial.print(F("0"));
      }
      Serial.print(serial[i], HEX);
    }
    Serial.println();
  }

  // Configure sensor (optional - defaults work well)
  tof.setDistanceMode(TMF8806_MODE_2_5M);  // 2.5m mode (default)
  tof.setIterations(400);                   // 400k iterations
  tof.setRepetitionPeriod(33);              // ~30Hz

  // Start continuous measurement
  if (!tof.startMeasuring(true)) {
    Serial.println(F("Failed to start measurement!"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("Measuring..."));
  Serial.println();
}

void loop() {
  if (tof.dataReady()) {
    tmf8806_result_t result;
    if (tof.readResult(&result) && result.reliability > 0) {
      Serial.print(F("Distance: "));
      Serial.print(result.distance);
      Serial.print(F(" mm"));

      Serial.print(F("  Reliability: "));
      Serial.print(result.reliability);

      Serial.print(F("  Temp: "));
      Serial.print(result.temperature);
      Serial.println(F(" C"));
    }
  }
  delay(10);
}
