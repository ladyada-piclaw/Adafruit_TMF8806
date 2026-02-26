/*!
 * @file Adafruit_TMF8806.h
 *
 * This is a library for the TMF8806 Time-of-Flight distance sensor.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 *
 * BSD license, all text here must be included in any redistribution.
 */

#ifndef ADAFRUIT_TMF8806_H
#define ADAFRUIT_TMF8806_H

#include <Adafruit_I2CDevice.h>
#include <Arduino.h>

#define TMF8806_DEFAULT_ADDR 0x41 ///< Default I2C address

// Common registers
#define TMF8806_REG_APPID 0x00        ///< Application ID register
#define TMF8806_REG_APPREV_MAJOR 0x01 ///< App major version
#define TMF8806_REG_APPREQID 0x02     ///< Request app ID
#define TMF8806_REG_APPREV_MINOR 0x12 ///< App minor version
#define TMF8806_REG_APPREV_PATCH 0x13 ///< App patch version
#define TMF8806_REG_CMD_DATA9 0x06    ///< Command data byte 9
#define TMF8806_REG_COMMAND 0x10      ///< Command register
#define TMF8806_REG_STATE 0x1C        ///< State register
#define TMF8806_REG_STATUS 0x1D       ///< Status register
#define TMF8806_REG_CONTENT 0x1E      ///< Register contents type
#define TMF8806_REG_CONFIG 0x20       ///< Config/calibration data start
#define TMF8806_REG_ENABLE 0xE0       ///< Enable register
#define TMF8806_REG_INT_STATUS 0xE1   ///< Interrupt status
#define TMF8806_REG_INT_ENAB 0xE2     ///< Interrupt enable
#define TMF8806_REG_ID 0xE3           ///< Chip ID register
#define TMF8806_REG_REVID 0xE4        ///< Chip revision ID register

// Application IDs
#define TMF8806_APP_BOOTLOADER 0x80  ///< Bootloader app ID
#define TMF8806_APP_MEASUREMENT 0xC0 ///< Measurement app ID

// Commands
#define TMF8806_CMD_MEASURE 0x02       ///< Start measurement
#define TMF8806_CMD_FACTORY_CALIB 0x0A ///< Factory calibration
#define TMF8806_CMD_READ_SERIAL 0x47   ///< Read serial number
#define TMF8806_CMD_STOP 0xFF          ///< Stop measurement

// Content types
#define TMF8806_CONTENT_CALIB 0x0A  ///< Calibration data
#define TMF8806_CONTENT_SERIAL 0x47 ///< Serial number
#define TMF8806_CONTENT_RESULT 0x55 ///< Measurement result

// Max distances per mode (mm)
#define TMF8806_MAX_SHORT_RANGE 200 ///< Short range max distance
#define TMF8806_MAX_2_5M 2650       ///< 2.5m mode max distance
#define TMF8806_MAX_5M 5300         ///< 5m mode max distance

// Factory calibration data size
#define TMF8806_CALIB_DATA_SIZE 14 ///< Factory calibration data size

// Expected chip ID (bits [5:0])
#define TMF8806_CHIP_ID 0x09 ///< Expected chip ID value

/*!
 * @brief Distance mode selection
 */
typedef enum {
  TMF8806_MODE_SHORT_RANGE = 0, ///< Short range (200mm max)
  TMF8806_MODE_2_5M = 1,        ///< 2.5m mode (default)
  TMF8806_MODE_5M = 2,          ///< 5m mode
} tmf8806_distance_mode_t;

/*!
 * @brief SPAD optical configuration
 */
typedef enum {
  TMF8806_SPAD_DEFAULT = 0,      ///< Default (0.5mm airgap, 0.55mm glass)
  TMF8806_SPAD_LARGE_AIRGAP = 1, ///< Large airgap (1mm, min 20mm)
  TMF8806_SPAD_THICK_GLASS = 2,  ///< Thick glass (3.2mm, min 40mm)
} tmf8806_spad_config_t;

/*!
 * @brief SPAD dead time settings
 */
typedef enum {
  TMF8806_SPAD_DEADTIME_97NS = 0, ///< 97ns (best short-range)
  TMF8806_SPAD_DEADTIME_48NS = 1, ///< 48ns
  TMF8806_SPAD_DEADTIME_32NS = 2, ///< 32ns
  TMF8806_SPAD_DEADTIME_24NS = 3, ///< 24ns
  TMF8806_SPAD_DEADTIME_16NS = 4, ///< 16ns (default balance)
  TMF8806_SPAD_DEADTIME_12NS = 5, ///< 12ns
  TMF8806_SPAD_DEADTIME_8NS = 6,  ///< 8ns
  TMF8806_SPAD_DEADTIME_4NS = 7,  ///< 4ns (best sunlight)
} tmf8806_spad_deadtime_t;

/*!
 * @brief GPIO mode selection
 */
typedef enum {
  TMF8806_GPIO_DISABLED = 0,           ///< GPIO disabled
  TMF8806_GPIO_INPUT_ACTIVE_LOW = 1,   ///< Input, low halts measurement
  TMF8806_GPIO_INPUT_ACTIVE_HIGH = 2,  ///< Input, high halts measurement
  TMF8806_GPIO_OUTPUT_VCSEL_PULSE = 3, ///< Output VCSEL timing
  TMF8806_GPIO_OUTPUT_LOW = 4,         ///< Output low
  TMF8806_GPIO_OUTPUT_HIGH = 5,        ///< Output high
  TMF8806_GPIO_OUTPUT_DETECT_HIGH = 6, ///< Output high on detect
  TMF8806_GPIO_OUTPUT_DETECT_LOW = 7,  ///< Output low on detect
  TMF8806_GPIO_OD_NO_DETECT_LOW = 8,   ///< Open-drain low on no detect
  TMF8806_GPIO_OD_DETECT_LOW = 9,      ///< Open-drain low on detect
} tmf8806_gpio_mode_t;

/*!
 * @brief Measurement result structure
 */
typedef struct {
  uint16_t distance;      ///< Distance in mm
  uint8_t reliability;    ///< Reliability 0-63 (63=best, 0=no object)
  uint8_t status;         ///< Measurement status (0-3)
  int8_t temperature;     ///< Die temperature in C
  uint32_t referenceHits; ///< Reference SPAD hit count
  uint32_t objectHits;    ///< Object SPAD hit count
  uint16_t crosstalk;     ///< Crosstalk value
} tmf8806_result_t;

/*!
 * @brief Adafruit TMF8806 Time-of-Flight sensor driver class
 */
class Adafruit_TMF8806 {
 public:
  Adafruit_TMF8806();
  ~Adafruit_TMF8806();

  bool begin(uint8_t addr = TMF8806_DEFAULT_ADDR, TwoWire* wire = &Wire);
  bool reset();

  // Measurement
  bool startMeasuring(bool continuous = true);
  bool stopMeasuring();
  bool dataReady();
  int16_t readDistance();
  bool readResult(tmf8806_result_t* result);

  // Configuration
  void setDistanceMode(tmf8806_distance_mode_t mode);
  void setIterations(uint16_t kIters);
  void setRepetitionPeriod(uint8_t periodMs);
  void setThreshold(uint8_t snrThreshold);
  void setSpadDeadTime(tmf8806_spad_deadtime_t dt);
  void setOpticalConfig(tmf8806_spad_config_t config);
  void setGPIOMode(uint8_t gpio, tmf8806_gpio_mode_t mode);

  // Calibration
  bool performFactoryCalibration();
  bool getCalibrationData(uint8_t* data, uint8_t len = TMF8806_CALIB_DATA_SIZE);
  bool setCalibrationData(const uint8_t* data,
                          uint8_t len = TMF8806_CALIB_DATA_SIZE);
  void enableCalibration(bool enable);

  // Info
  uint8_t getChipID();
  uint8_t getRevisionID();
  int8_t getTemperature();
  void getVersion(uint8_t* major, uint8_t* minor, uint8_t* patch);
  bool readSerialNumber(uint8_t* serial, uint8_t len);

 private:
  Adafruit_I2CDevice* _i2c_dev; ///< I2C device object

  // Configuration state
  tmf8806_distance_mode_t _distanceMode; ///< Current distance mode
  uint16_t _kIters;                      ///< Iterations (units of 1000)
  uint8_t _repetitionPeriod;             ///< Repetition period in ms
  uint8_t _snrThreshold;                 ///< SNR threshold
  tmf8806_spad_deadtime_t _spadDeadTime; ///< SPAD dead time
  tmf8806_spad_config_t _spadConfig;     ///< SPAD optical config
  uint8_t _gpio0Mode;                    ///< GPIO0 mode
  uint8_t _gpio1Mode;                    ///< GPIO1 mode

  // Calibration state
  bool _useCalibration;                        ///< Use calibration data
  uint8_t _calibData[TMF8806_CALIB_DATA_SIZE]; ///< Calibration data
  bool _hasCalibData;                          ///< Have valid cal data

  // Last result temperature
  int8_t _lastTemperature; ///< Last measured temperature

  // Internal methods
  bool waitForCpuReady(uint16_t timeoutMs);
  bool waitForApp(uint16_t timeoutMs);
  bool executeCommand(uint8_t cmd, uint16_t timeoutMs);
  bool writeReg(uint8_t reg, uint8_t value);
  uint8_t readReg(uint8_t reg);
  bool readRegBuffer(uint8_t reg, uint8_t* buffer, uint8_t len);
  bool writeRegBuffer(uint8_t reg, uint8_t* buffer, uint8_t len);
  uint16_t getMaxDistance();
};

#endif // ADAFRUIT_TMF8806_H
