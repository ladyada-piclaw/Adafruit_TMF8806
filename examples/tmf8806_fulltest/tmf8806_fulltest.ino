/*!
 * @file tmf8806_fulltest.ino
 *
 * Full test sketch for TMF8806 Time-of-Flight Distance Sensor.
 * Sets and reads back all available configurations.
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

  Serial.println(F("TMF8806 Time-of-Flight Sensor Full Test"));

  if (!tof.begin()) {
    Serial.println(F("Failed to find TMF8806 chip"));
    while (1)
      delay(10);
  }

  Serial.println(F("TMF8806 found!"));

  // --- Device Info ---
  Serial.print(F("Chip ID: 0x"));
  Serial.println(tof.getChipID(), HEX);

  Serial.print(F("Revision ID: "));
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
      if (serial[i] < 0x10)
        Serial.print(F("0"));
      Serial.print(serial[i], HEX);
    }
    Serial.println();
  }

  // --- Distance Mode ---
  tof.setDistanceMode(TMF8806_MODE_2_5M);
  // Other options:
  // tof.setDistanceMode(TMF8806_MODE_SHORT_RANGE); // max 200mm
  // tof.setDistanceMode(TMF8806_MODE_5M);          // max 5300mm
  Serial.print(F("Distance mode: "));
  switch (tof.getDistanceMode()) {
    case TMF8806_MODE_SHORT_RANGE:
      Serial.println(F("Short Range (max 200mm)"));
      break;
    case TMF8806_MODE_2_5M:
      Serial.println(F("2.5m (max 2650mm)"));
      break;
    case TMF8806_MODE_5M:
      Serial.println(F("5m (max 5300mm)"));
      break;
  }

  // --- Iterations ---
  tof.setIterations(400); // units of 1000, range 10-4000
  // Other options:
  // tof.setIterations(50);   // fastest, lowest accuracy
  // tof.setIterations(900);  // slower, best accuracy
  Serial.print(F("Iterations: "));
  Serial.print(tof.getIterations());
  Serial.println(F("k"));

  // --- Repetition Period ---
  tof.setRepetitionPeriod_ms(33); // ms between measurements
  // Other options:
  // tof.setRepetitionPeriod_ms(0);    // single-shot mode
  // tof.setRepetitionPeriod_ms(10);   // fast (~100 Hz if iterations allow)
  // tof.setRepetitionPeriod_ms(66);   // ~15 Hz
  // tof.setRepetitionPeriod_ms(100);  // ~10 Hz
  // tof.setRepetitionPeriod_ms(254);  // 1 second
  // tof.setRepetitionPeriod_ms(255);  // 2 seconds
  Serial.print(F("Repetition period: "));
  Serial.print(tof.getRepetitionPeriod_ms());
  Serial.println(F(" ms"));

  // --- SNR Threshold ---
  tof.setSNRThreshold(6); // 0-63, higher = fewer false detects
  // Other options:
  // tof.setSNRThreshold(0);   // no filtering
  // tof.setSNRThreshold(12);  // stricter, may miss weak targets
  Serial.print(F("SNR threshold: "));
  Serial.println(tof.getSNRThreshold());

  // --- SPAD Dead Time ---
  tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_97NS);
  // Other options:
  // tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_48NS);
  // tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_32NS);
  // tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_24NS);
  // tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_16NS);  // default balance
  // tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_12NS);
  // tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_8NS);
  // tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_4NS);   // best sunlight
  // 97ns = best short-range accuracy, 4ns = best sunlight performance
  Serial.print(F("SPAD dead time: "));
  switch (tof.getSpadDeadTime()) {
    case TMF8806_SPAD_DEADTIME_97NS:
      Serial.println(F("97ns"));
      break;
    case TMF8806_SPAD_DEADTIME_48NS:
      Serial.println(F("48ns"));
      break;
    case TMF8806_SPAD_DEADTIME_32NS:
      Serial.println(F("32ns"));
      break;
    case TMF8806_SPAD_DEADTIME_24NS:
      Serial.println(F("24ns"));
      break;
    case TMF8806_SPAD_DEADTIME_16NS:
      Serial.println(F("16ns"));
      break;
    case TMF8806_SPAD_DEADTIME_12NS:
      Serial.println(F("12ns"));
      break;
    case TMF8806_SPAD_DEADTIME_8NS:
      Serial.println(F("8ns"));
      break;
    case TMF8806_SPAD_DEADTIME_4NS:
      Serial.println(F("4ns"));
      break;
  }

  // --- Optical Configuration ---
  tof.setOpticalConfig(TMF8806_SPAD_DEFAULT);
  // Other options:
  // tof.setOpticalConfig(TMF8806_SPAD_LARGE_AIRGAP);  // 1mm airgap, min 20mm
  // tof.setOpticalConfig(TMF8806_SPAD_THICK_GLASS);    // 3.2mm glass, min 40mm
  Serial.print(F("Optical config: "));
  switch (tof.getOpticalConfig()) {
    case TMF8806_SPAD_DEFAULT:
      Serial.println(F("Default (0.5mm airgap, 0.55mm glass)"));
      break;
    case TMF8806_SPAD_LARGE_AIRGAP:
      Serial.println(F("Large Airgap (1mm, min 20mm)"));
      break;
    case TMF8806_SPAD_THICK_GLASS:
      Serial.println(F("Thick Glass (3.2mm, min 40mm)"));
      break;
  }

  // --- GPIO Modes ---
  // clang-format off
  tof.setGPIOMode(0, TMF8806_GPIO_DISABLED);
  // Other options:
  // tof.setGPIOMode(0, TMF8806_GPIO_INPUT_ACTIVE_LOW);   // low halts measure
  // tof.setGPIOMode(0, TMF8806_GPIO_INPUT_ACTIVE_HIGH);  // high halts measure
  // tof.setGPIOMode(0, TMF8806_GPIO_OUTPUT_VCSEL_PULSE); // VCSEL timing
  // tof.setGPIOMode(0, TMF8806_GPIO_OUTPUT_LOW);         // drive low
  // tof.setGPIOMode(0, TMF8806_GPIO_OUTPUT_HIGH);        // drive high
  // tof.setGPIOMode(0, TMF8806_GPIO_OUTPUT_DETECT_HIGH); // high on detect
  // tof.setGPIOMode(0, TMF8806_GPIO_OUTPUT_DETECT_LOW);  // low on detect
  // tof.setGPIOMode(0, TMF8806_GPIO_OD_NO_DETECT_LOW);   // OD low no detect
  // tof.setGPIOMode(0, TMF8806_GPIO_OD_DETECT_LOW);      // OD low on detect
  // Note: GPIO modes are applied when startMeasuring() is called.
  // clang-format on
  Serial.print(F("GPIO0: "));
  Serial.println(tof.getGPIOMode(0) == TMF8806_GPIO_DISABLED ? F("disabled")
                                                             : F("enabled"));

  tof.setGPIOMode(1, TMF8806_GPIO_DISABLED);
  // Same options as GPIO0 above
  Serial.print(F("GPIO1: "));
  Serial.println(tof.getGPIOMode(1) == TMF8806_GPIO_DISABLED ? F("disabled")
                                                             : F("enabled"));

  // --- Calibration ---
  // Uncomment to run factory calibration (needs clear FoV >40cm)
  /*
  Serial.println(F("Running factory calibration..."));
  if (tof.performFactoryCalibration()) {
    uint8_t calData[14];
    tof.getCalibrationData(calData, 14);
    Serial.print(F("Cal data: "));
    for (uint8_t i = 0; i < 14; i++) {
      if (calData[i] < 0x10) Serial.print(F("0"));
      Serial.print(calData[i], HEX);
      Serial.print(F(" "));
    }
    Serial.println();

    // To use saved calibration data:
    // tof.setCalibrationData(calData, 14);
    // tof.enableCalibration(true);
  }
  */

  Serial.println();

  // --- Start measuring ---
  if (!tof.startMeasuring(true)) {
    Serial.println(F("Failed to start measurement!"));
    while (1)
      delay(10);
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
      Serial.print(F("/63"));

      Serial.print(F("  Temp: "));
      Serial.print(result.temperature);
      Serial.print(F(" C"));

      Serial.print(F("  Ref hits: "));
      Serial.print(result.referenceHits);

      Serial.print(F("  Obj hits: "));
      Serial.print(result.objectHits);

      Serial.print(F("  Xtalk: "));
      Serial.println(result.crosstalk);
    }
  }
  delay(10);
}
