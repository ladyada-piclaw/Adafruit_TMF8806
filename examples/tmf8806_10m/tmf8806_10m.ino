/*!
 * @file tmf8806_10m.ino
 *
 * Example demonstrating 10m extended range mode on the TMF8806.
 * This requires downloading a firmware patch to the sensor which
 * extends the measurement range from 5m to 10m (10600mm max).
 *
 * NOTE: 10m mode is not fully verified. The firmware patch from the
 * ams-OSRAM reference driver (v4.14.11) may not actually enable
 * distances beyond ~5m. A newer firmware image from ams-OSRAM may
 * be required for true 10m operation.
 *
 * The firmware patch is stored in PROGMEM (~2.9KB) and downloaded
 * to the sensor's RAM on startup. It needs to be re-downloaded
 * after each power cycle.
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * BSD license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8806.h>

Adafruit_TMF8806 tmf8806;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8806 10m Mode Example"));
  Serial.println(F("========================"));

  // Initialize I2C
  Wire.begin();

  // Initialize sensor
  Serial.print(F("Initializing TMF8806... "));
  if (!tmf8806.begin()) {
    Serial.println(F("FAILED!"));
    Serial.println(F("Check wiring and I2C address."));
    while (1) {
      delay(100);
    }
  }
  Serial.println(F("OK"));

  // Print version info
  uint8_t major, minor, patch;
  tmf8806.getVersion(&major, &minor, &patch);
  Serial.print(F("Firmware version: "));
  Serial.print(major);
  Serial.print(F("."));
  Serial.print(minor);
  Serial.print(F("."));
  Serial.println(patch);

  // Download firmware patch for 10m mode
  Serial.print(F("Downloading firmware patch"));
  unsigned long startMs = millis();
  if (!tmf8806.downloadFirmware()) {
    Serial.println(F(" FAILED!"));
    Serial.println(F("Firmware download failed."));
    while (1) {
      delay(100);
    }
  }
  unsigned long downloadTime = millis() - startMs;
  Serial.print(F(" OK ("));
  Serial.print(downloadTime);
  Serial.println(F("ms)"));

  // Print patched version
  tmf8806.getVersion(&major, &minor, &patch);
  Serial.print(F("Patched firmware version: "));
  Serial.print(major);
  Serial.print(F("."));
  Serial.print(minor);
  Serial.print(F("."));
  Serial.println(patch);

  // Perform factory calibration
  // Note: For best results, ensure no object within 40cm of sensor
  Serial.print(F("Performing factory calibration... "));
  if (!tmf8806.performFactoryCalibration()) {
    Serial.println(F("FAILED!"));
    Serial.println(F("Continuing without calibration."));
  } else {
    Serial.println(F("OK"));
    tmf8806.enableCalibration(true);
  }

  // Set 10m mode
  tmf8806.setDistanceMode(TMF8806_MODE_10M);
  Serial.println(F("Distance mode: 10m (10600mm max)"));

  // Use higher iterations for better accuracy at long range
  tmf8806.setIterations(4000);
  Serial.println(F("Iterations: 4000k"));

  // Set repetition period (~10Hz)
  tmf8806.setRepetitionPeriod_ms(100);

  // Start continuous measurement
  Serial.print(F("Starting measurements... "));
  if (!tmf8806.startMeasuring(true)) {
    Serial.println(F("FAILED!"));
    while (1) {
      delay(100);
    }
  }
  Serial.println(F("OK"));
  Serial.println();
  Serial.println(F("Distance (mm) | Reliability | Temp (C)"));
  Serial.println(F("----------------------------------------"));
}

void loop() {
  if (tmf8806.dataReady()) {
    tmf8806_result_t result;
    if (tmf8806.readResult(&result)) {
      // Print distance
      if (result.reliability > 0) {
        Serial.print(result.distance);
        Serial.print(F(" mm"));
      } else {
        Serial.print(F("No object"));
      }

      // Pad to column
      if (result.distance < 100) {
        Serial.print(F("      "));
      } else if (result.distance < 1000) {
        Serial.print(F("     "));
      } else if (result.distance < 10000) {
        Serial.print(F("    "));
      } else {
        Serial.print(F("   "));
      }

      Serial.print(F(" | "));
      Serial.print(result.reliability);
      Serial.print(F("/63"));
      if (result.reliability < 10) {
        Serial.print(F(" "));
      }

      Serial.print(F("       | "));
      Serial.print(result.temperature);
      Serial.println(F(" C"));
    }
  }
}
