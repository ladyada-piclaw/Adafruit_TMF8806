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
  Serial.print(F("Distance mode: "));
  Serial.println(F("2.5m"));

  // Other options:
  // tof.setDistanceMode(TMF8806_MODE_SHORT_RANGE); // max 200mm
  // tof.setDistanceMode(TMF8806_MODE_5M);          // max 5300mm

  // --- Iterations ---
  tof.setIterations(400); // units of 1000, range 10-4000
  Serial.print(F("Iterations: "));
  Serial.println(F("400k"));

  // Higher = more accurate + higher reliability, but slower
  // 50k  = fastest, lowest accuracy
  // 400k = default, good balance
  // 900k = slower, best accuracy

  // --- Repetition Period ---
  tof.setRepetitionPeriod(33); // ms, 0 = single-shot
  Serial.print(F("Repetition period: "));
  Serial.println(F("33 ms (~30 Hz)"));

  // Other options:
  // tof.setRepetitionPeriod(0);    // single-shot
  // tof.setRepetitionPeriod(10);   // fast (~100 Hz if iterations allow)
  // tof.setRepetitionPeriod(66);   // ~15 Hz
  // tof.setRepetitionPeriod(100);  // ~10 Hz
  // tof.setRepetitionPeriod(254);  // 1 second
  // tof.setRepetitionPeriod(255);  // 2 seconds

  // --- SNR Threshold ---
  tof.setThreshold(6); // 0-63, default 6
  Serial.print(F("SNR threshold: "));
  Serial.println(F("6 (default)"));

  // Higher threshold = fewer false detections but may miss weak targets

  // --- SPAD Dead Time ---
  tof.setSpadDeadTime(TMF8806_SPAD_DEADTIME_97NS);
  Serial.print(F("SPAD dead time: "));
  Serial.println(F("97ns (best short-range accuracy)"));

  // Options:
  // TMF8806_SPAD_DEADTIME_97NS  = best short-range accuracy
  // TMF8806_SPAD_DEADTIME_48NS
  // TMF8806_SPAD_DEADTIME_32NS
  // TMF8806_SPAD_DEADTIME_24NS
  // TMF8806_SPAD_DEADTIME_16NS  = default balance
  // TMF8806_SPAD_DEADTIME_12NS
  // TMF8806_SPAD_DEADTIME_8NS
  // TMF8806_SPAD_DEADTIME_4NS   = best sunlight performance

  // --- Optical Configuration ---
  tof.setOpticalConfig(TMF8806_SPAD_DEFAULT);
  Serial.print(F("Optical config: "));
  Serial.println(F("Default (0.5mm airgap, 0.55mm glass)"));

  // Options:
  // TMF8806_SPAD_DEFAULT       = 0.5mm airgap, 0.55mm glass, min 0mm
  // TMF8806_SPAD_LARGE_AIRGAP  = 1mm airgap, min 20mm
  // TMF8806_SPAD_THICK_GLASS   = 0mm airgap, 3.2mm glass, min 40mm

  // --- GPIO Modes ---
  tof.setGPIOMode(0, TMF8806_GPIO_DISABLED);
  Serial.print(F("GPIO0: "));
  Serial.println(F("disabled"));

  tof.setGPIOMode(1, TMF8806_GPIO_DISABLED);
  Serial.print(F("GPIO1: "));
  Serial.println(F("disabled"));

  // GPIO options:
  // TMF8806_GPIO_DISABLED            = off
  // TMF8806_GPIO_INPUT_ACTIVE_LOW    = low halts measurement
  // TMF8806_GPIO_INPUT_ACTIVE_HIGH   = high halts measurement
  // TMF8806_GPIO_OUTPUT_VCSEL_PULSE  = VCSEL timing signal
  // TMF8806_GPIO_OUTPUT_LOW          = output low
  // TMF8806_GPIO_OUTPUT_HIGH         = output high
  // TMF8806_GPIO_OUTPUT_DETECT_HIGH  = high when object detected
  // TMF8806_GPIO_OUTPUT_DETECT_LOW   = low when object detected
  // TMF8806_GPIO_OD_NO_DETECT_LOW    = open-drain, low on no detect
  // TMF8806_GPIO_OD_DETECT_LOW       = open-drain, low on detect
  // Note: GPIO modes are applied when startMeasuring() is called.
  // GPIO0 is also used for I/O voltage detection at startup.

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
