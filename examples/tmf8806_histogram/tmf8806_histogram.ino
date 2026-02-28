/*!
 * @file tmf8806_histogram.ino
 *
 * Example demonstrating histogram readout from the TMF8806 ToF sensor.
 * Displays an ASCII bar chart of the histogram data on the serial monitor.
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * BSD license, all text here must be included in any redistribution.
 */

#include <Adafruit_TMF8806.h>

Adafruit_TMF8806 tmf;
uint16_t histogramBins[TMF8806_HISTOGRAM_BINS];
tmf8806_histogram_type_t currentHistType = TMF8806_HIST_DISTANCE;
bool histogramEnabled = false;

void printMenu() {
  Serial.println();
  Serial.println(F("TMF8806 Histogram Viewer"));
  Serial.println(F("========================"));
  Serial.println(F("Select histogram type:"));
  Serial.println(F("  1) Electrical Calibration"));
  Serial.println(F("  2) Proximity"));
  Serial.println(F("  3) Distance"));
  Serial.println(F("  4) Pile-up Corrected"));
  Serial.println(F("  5) Pile-up TDC Sum"));
  Serial.println(F("  0) Stop histogram"));
  Serial.println(F("Enter choice: "));
}

void printHistogram(const char *title) {
  // Find max value for scaling
  uint16_t maxVal = 0;
  uint8_t peakBin = 0;
  for (uint8_t i = 0; i < TMF8806_HISTOGRAM_BINS; i++) {
    if (histogramBins[i] > maxVal) {
      maxVal = histogramBins[i];
      peakBin = i;
    }
  }

  Serial.println();
  Serial.print(F("--- "));
  Serial.print(title);
  Serial.println(F(" ---"));

  // Print every 4th bin to keep output manageable (32 lines)
  for (uint8_t i = 0; i < TMF8806_HISTOGRAM_BINS; i += 4) {
    // Print bin number (right-justified, 3 chars)
    if (i < 10)
      Serial.print(F("  "));
    else if (i < 100)
      Serial.print(F(" "));
    Serial.print(i);
    Serial.print(F(": |"));

    // Calculate bar width (max 50 chars)
    uint8_t barWidth = 0;
    if (maxVal > 0) {
      barWidth = (uint8_t)(((uint32_t)histogramBins[i] * 50) / maxVal);
    }

    // Print bar
    for (uint8_t j = 0; j < barWidth; j++) {
      Serial.print(F("#"));
    }

    // Pad to fixed width and print value
    for (uint8_t j = barWidth; j < 50; j++) {
      Serial.print(F(" "));
    }
    Serial.print(F(" "));
    Serial.println(histogramBins[i]);
  }

  Serial.print(F("Peak: bin "));
  Serial.print(peakBin);
  Serial.print(F(" = "));
  Serial.println(maxVal);
}

const char *getHistTypeName(tmf8806_histogram_type_t type) {
  switch (type) {
  case TMF8806_HIST_ELECTRICAL_CAL:
    return "Electrical Calibration";
  case TMF8806_HIST_PROXIMITY:
    return "Proximity";
  case TMF8806_HIST_DISTANCE:
    return "Distance";
  case TMF8806_HIST_PILEUP:
    return "Pile-up Corrected";
  case TMF8806_HIST_PILEUP_TDC_SUM:
    return "Pile-up TDC Sum";
  default:
    return "Unknown";
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println(F("TMF8806 Histogram Example"));
  Serial.println();

  if (!tmf.begin()) {
    Serial.println(F("Failed to find TMF8806 sensor!"));
    while (1)
      delay(10);
  }
  Serial.println(F("TMF8806 Found!"));

  // Get version info
  uint8_t major, minor, patch;
  tmf.getVersion(&major, &minor, &patch);
  Serial.print(F("Firmware: "));
  Serial.print(major);
  Serial.print(F("."));
  Serial.print(minor);
  Serial.print(F("."));
  Serial.println(patch);

  // Run factory calibration
  Serial.println(F("Running factory calibration..."));
  if (tmf.performFactoryCalibration()) {
    Serial.println(F("Calibration complete!"));
    tmf.enableCalibration(true);
  } else {
    Serial.println(F("Calibration failed, continuing without"));
  }

  printMenu();
}

void loop() {
  // Check for serial input
  if (Serial.available()) {
    char c = Serial.read();
    if (c >= '0' && c <= '5') {
      // Stop any current measurement
      if (histogramEnabled) {
        tmf.stopMeasuring();
        tmf.disableHistogram();
        histogramEnabled = false;
      }

      if (c == '0') {
        Serial.println(F("Histogram stopped."));
        printMenu();
      } else {
        // Map menu choice to histogram type
        switch (c) {
        case '1':
          currentHistType = TMF8806_HIST_ELECTRICAL_CAL;
          break;
        case '2':
          currentHistType = TMF8806_HIST_PROXIMITY;
          break;
        case '3':
          currentHistType = TMF8806_HIST_DISTANCE;
          break;
        case '4':
          currentHistType = TMF8806_HIST_PILEUP;
          break;
        case '5':
          currentHistType = TMF8806_HIST_PILEUP_TDC_SUM;
          break;
        }

        Serial.print(F("Enabling "));
        Serial.print(getHistTypeName(currentHistType));
        Serial.println(F(" histogram..."));

        if (tmf.configureHistogram(currentHistType)) {
          Serial.println(F("Histogram configured, starting measurement..."));
          if (tmf.startMeasuring(true)) {
            histogramEnabled = true;
            Serial.println(F("Measuring... (press 0 to stop)"));
          } else {
            Serial.println(F("Failed to start measurement!"));
            printMenu();
          }
        } else {
          Serial.println(F("Failed to configure histogram!"));
          printMenu();
        }
      }
    }
  }

  // If histogram enabled, check for data
  if (histogramEnabled) {
    // Check for distance result first
    if (tmf.dataReady()) {
      tmf8806_result_t result;
      if (tmf.readResult(&result)) {
        Serial.print(F("Distance: "));
        Serial.print(result.distance);
        Serial.print(F(" mm (reliability: "));
        Serial.print(result.reliability);
        Serial.println(F(")"));
      }
    }

    // Check for histogram ready
    if (tmf.histogramReady()) {
      if (tmf.readHistogram(histogramBins)) {
        printHistogram(getHistTypeName(currentHistType));
      } else {
        Serial.println(F("Failed to read histogram"));
      }
    }
  }

  delay(50);
}
