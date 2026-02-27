/*!
 * @file Adafruit_TMF8806.cpp
 *
 * @mainpage Adafruit TMF8806 Time-of-Flight Distance Sensor Library
 *
 * @section intro_sec Introduction
 *
 * This is a library for the TMF8806 Time-of-Flight distance sensor.
 * The TMF8806 is a direct Time-of-Flight (dToF) sensor capable of
 * measuring distances from 10mm to 5000mm (or 10m with firmware patch).
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 */

#include "Adafruit_TMF8806.h"

/*!
 * @brief Constructor for Adafruit_TMF8806
 */
Adafruit_TMF8806::Adafruit_TMF8806() {
  _i2c_dev = nullptr;
  _distanceMode = TMF8806_MODE_2_5M;
  _kIters = 400;          // Default 400k iterations
  _repetitionPeriod = 33; // Default ~30Hz
  _snrThreshold = 6;      // Default threshold
  _spadDeadTime = TMF8806_SPAD_DEADTIME_97NS;
  _spadConfig = TMF8806_SPAD_DEFAULT;
  _gpio0Mode = 0;
  _gpio1Mode = 0;
  _useCalibration = false;
  _hasCalibData = false;
  _lastTemperature = 0;
  memset(_calibData, 0, TMF8806_CALIB_DATA_SIZE);
}

/*!
 * @brief Destructor for Adafruit_TMF8806
 */
Adafruit_TMF8806::~Adafruit_TMF8806() {
  if (_i2c_dev) {
    delete _i2c_dev;
  }
}

/*!
 * @brief Initialize the TMF8806 sensor
 * @param addr I2C address (default 0x41)
 * @param wire Pointer to TwoWire instance (default &Wire)
 * @return true on success, false on failure
 */
bool Adafruit_TMF8806::begin(uint8_t addr, TwoWire* wire) {
  if (_i2c_dev) {
    delete _i2c_dev;
  }
  _i2c_dev = new Adafruit_I2CDevice(addr, wire);

  if (!_i2c_dev->begin()) {
    return false;
  }

  // Boot into measurement app
  if (!startApp()) {
    return false;
  }

  // Verify chip ID (bits [5:0] should be 0x09)
  if (getChipID() != TMF8806_CHIP_ID) {
    return false;
  }

  return true;
}

/*!
 * @brief Software reset the sensor
 * @return true on success, false on failure
 */
bool Adafruit_TMF8806::reset() {
  // Trigger soft reset via RESETREASON register bit 7
  Adafruit_BusIO_Register resetreason_reg =
      Adafruit_BusIO_Register(_i2c_dev, 0xF0);
  Adafruit_BusIO_RegisterBits reset_bit =
      Adafruit_BusIO_RegisterBits(&resetreason_reg, 1, 7);
  if (!reset_bit.write(1)) {
    return false;
  }

  delay(5);

  return startApp();
}

/*!
 * @brief Start distance measurement
 * @param continuous If true, measure continuously; if false, single-shot
 * @return true on success, false on failure
 */
bool Adafruit_TMF8806::startMeasuring(bool continuous) {
  uint8_t cmdData[11];

  // If using calibration, write cal data to 0x20 first
  if (_useCalibration && _hasCalibData) {
    uint8_t reg = TMF8806_REG_CONFIG;
    if (!_i2c_dev->write(_calibData, TMF8806_CALIB_DATA_SIZE, true, &reg, 1)) {
      return false;
    }
  }

  // Build command data (registers 0x06-0x10)
  // CMD_DATA9 (0x06): Spread spectrum SPAD - 0x00
  cmdData[0] = 0x00;

  // CMD_DATA8 (0x07): Spread spectrum VCSEL - 0x00
  cmdData[1] = 0x00;

  // CMD_DATA7 (0x08): Calibration/SPAD settings
  // bit 0: factoryCal (1 if using calibration)
  // bit 1: algState (0)
  // bit 2: reserved (0)
  // bits 5:3: spadDeadTime
  // bits 7:6: spadSelect
  cmdData[2] = ((_useCalibration && _hasCalibData) ? 0x01 : 0x00) |
               (((uint8_t)_spadDeadTime & 0x07) << 3) |
               (((uint8_t)_spadConfig & 0x03) << 6);

  // CMD_DATA6 (0x09): Algorithm settings
  // bit 0: reserved (0)
  // bit 1: distanceEnabled (1 for full range)
  // bit 2: vcselClkDiv2 (1 for 5m mode)
  // bit 3: distanceMode (1 for 5m mode)
  // bit 4: immediateInterrupt (0)
  // bits 6:5: reserved (0)
  // bit 7: algKeepReady (0)
  uint8_t algoConfig = 0x02; // distanceEnabled = 1
  if (_distanceMode == TMF8806_MODE_5M) {
    algoConfig |= 0x0C; // vcselClkDiv2=1, distanceMode=1
  } else if (_distanceMode == TMF8806_MODE_SHORT_RANGE) {
    algoConfig = 0x00; // distanceEnabled = 0 for short range only
  }
  cmdData[3] = algoConfig;

  // CMD_DATA5 (0x0A): GPIO settings
  cmdData[4] = (_gpio0Mode & 0x0F) | ((_gpio1Mode & 0x0F) << 4);

  // CMD_DATA4 (0x0B): DAX delay - 0x00
  cmdData[5] = 0x00;

  // CMD_DATA3 (0x0C): SNR threshold + spread spectrum
  cmdData[6] = _snrThreshold & 0x3F;

  // CMD_DATA2 (0x0D): Repetition period
  cmdData[7] = continuous ? _repetitionPeriod : 0;

  // CMD_DATA1 (0x0E): kIters LSB
  cmdData[8] = _kIters & 0xFF;

  // CMD_DATA0 (0x0F): kIters MSB
  cmdData[9] = (_kIters >> 8) & 0xFF;

  // COMMAND (0x10): Measure command
  cmdData[10] = TMF8806_CMD_MEASURE;

  // Write all 11 bytes starting at 0x06
  uint8_t reg = TMF8806_REG_CMD_DATA9;
  if (!_i2c_dev->write(cmdData, 11, true, &reg, 1)) {
    return false;
  }

  // Wait for command to be accepted
  return executeCommand(TMF8806_CMD_MEASURE, 5);
}

/*!
 * @brief Stop measurement
 * @return true on success, false on failure
 */
bool Adafruit_TMF8806::stopMeasuring() {
  Adafruit_BusIO_Register command_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_COMMAND);
  if (!command_reg.write(TMF8806_CMD_STOP)) {
    return false;
  }

  // Timeout = 5 + (kIters / 18) ms
  uint16_t timeout = 5 + (_kIters / 18);
  return executeCommand(TMF8806_CMD_STOP, timeout);
}

/*!
 * @brief Check if measurement data is ready
 * @return true if data ready, false otherwise
 */
bool Adafruit_TMF8806::dataReady() {
  Adafruit_BusIO_Register int_status_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_INT_STATUS);
  Adafruit_BusIO_RegisterBits int1_status =
      Adafruit_BusIO_RegisterBits(&int_status_reg, 1, 0);
  return int1_status.read();
}

/*!
 * @brief Read distance (convenience function)
 * @return Distance in mm, or -1 on error
 */
int16_t Adafruit_TMF8806::readDistance() {
  if (!dataReady()) {
    return -1;
  }

  tmf8806_result_t result;
  if (!readResult(&result)) {
    return -1;
  }

  return (int16_t)result.distance;
}

/*!
 * @brief Read full measurement result
 * @param result Pointer to result structure to fill
 * @return true on success, false on failure
 */
bool Adafruit_TMF8806::readResult(tmf8806_result_t* result) {
  uint8_t buffer[34]; // Read from 0x1C to 0x3D (34 bytes)

  uint8_t regState = TMF8806_REG_STATE;
  if (!_i2c_dev->write_then_read(&regState, 1, buffer, 34)) {
    return false;
  }

  // Check CONTENT register (offset 2, register 0x1E) == 0x55
  if (buffer[2] != TMF8806_CONTENT_RESULT) {
    return false;
  }

  // Clear interrupt
  Adafruit_BusIO_Register int_status_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_INT_STATUS);
  Adafruit_BusIO_RegisterBits int1_status =
      Adafruit_BusIO_RegisterBits(&int_status_reg, 1, 0);
  int1_status.write(1);

  // Parse result from offset 4 (register 0x20)
  // Result structure starts at 0x20, but our buffer starts at 0x1C
  // Offset 4 = RESULT_NUMBER (0x20)
  // Offset 5 = RESULT_INFO (0x21) - reliability in bits [5:0], status in [7:6]
  // Offset 6-7 = DISTANCE (0x22-0x23) little-endian
  // Offset 8-11 = SYS_CLOCK (0x24-0x27)
  // Offset 12-22 = STATE_DATA (0x28-0x32)
  // Offset 23 = TEMPERATURE (0x33)
  // Offset 24-27 = REFERENCE_HITS (0x34-0x37)
  // Offset 28-31 = OBJECT_HITS (0x38-0x3B)
  // Offset 32-33 = XTALK (0x3C-0x3D)

  result->reliability = buffer[5] & 0x3F;
  result->status = (buffer[5] >> 6) & 0x03;

  // Distance (little-endian)
  result->distance = buffer[6] | ((uint16_t)buffer[7] << 8);

  // Clamp to max distance for mode
  uint16_t maxDist = getMaxDistance();
  if (result->distance > maxDist) {
    result->distance = 0;
    result->reliability = 0;
  }

  // Temperature
  result->temperature = (int8_t)buffer[23];
  _lastTemperature = result->temperature;

  // Reference hits (little-endian 32-bit)
  result->referenceHits = buffer[24] | ((uint32_t)buffer[25] << 8) |
                          ((uint32_t)buffer[26] << 16) |
                          ((uint32_t)buffer[27] << 24);

  // Object hits (little-endian 32-bit)
  result->objectHits = buffer[28] | ((uint32_t)buffer[29] << 8) |
                       ((uint32_t)buffer[30] << 16) |
                       ((uint32_t)buffer[31] << 24);

  // Crosstalk (little-endian 16-bit)
  result->crosstalk = buffer[32] | ((uint16_t)buffer[33] << 8);

  return true;
}

/*!
 * @brief Set distance mode
 * @param mode Distance mode (short range, 2.5m, or 5m)
 */
void Adafruit_TMF8806::setDistanceMode(tmf8806_distance_mode_t mode) {
  _distanceMode = mode;
}

/*!
 * @brief Set number of integration iterations
 * @param kIters Number of iterations in units of 1000 (10-4000)
 */
void Adafruit_TMF8806::setIterations(uint16_t kIters) {
  if (kIters < 10)
    kIters = 10;
  if (kIters > 4000)
    kIters = 4000;
  _kIters = kIters;
}

/*!
 * @brief Set measurement repetition period
 * @param periodMs Period in milliseconds (5-253, 0=single-shot)
 */
void Adafruit_TMF8806::setRepetitionPeriod(uint8_t periodMs) {
  _repetitionPeriod = periodMs;
}

/*!
 * @brief Set SNR detection threshold
 * @param snrThreshold Threshold value (0=default which is 6)
 */
void Adafruit_TMF8806::setThreshold(uint8_t snrThreshold) {
  _snrThreshold = snrThreshold & 0x3F;
}

/*!
 * @brief Set SPAD dead time
 * @param dt Dead time setting
 */
void Adafruit_TMF8806::setSpadDeadTime(tmf8806_spad_deadtime_t dt) {
  _spadDeadTime = dt;
}

/*!
 * @brief Set optical stack configuration
 * @param config SPAD optical configuration
 */
void Adafruit_TMF8806::setOpticalConfig(tmf8806_spad_config_t config) {
  _spadConfig = config;
}

/*!
 * @brief Set GPIO mode (applied on next startMeasuring() call)
 *
 * GPIO configuration is part of the measurement command and takes effect
 * when startMeasuring() is called. There is no standalone GPIO command.
 *
 * @note GPIO0 is used for I/O voltage auto-detection at startup (EN rising
 * edge). Ensure any external pull-up on GPIO0 does not conflict with the
 * desired output mode.
 *
 * @param gpio GPIO number (0 or 1)
 * @param mode GPIO mode
 */
void Adafruit_TMF8806::setGPIOMode(uint8_t gpio, tmf8806_gpio_mode_t mode) {
  if (gpio == 0) {
    _gpio0Mode = (uint8_t)mode;
  } else if (gpio == 1) {
    _gpio1Mode = (uint8_t)mode;
  }
}

/*!
 * @brief Perform factory calibration
 * @return true on success, false on failure
 */
bool Adafruit_TMF8806::performFactoryCalibration() {
  uint8_t cmdData[11];

  // Build calibration command with 4000k iterations
  cmdData[0] = 0x00; // Spread spectrum SPAD
  cmdData[1] = 0x00; // Spread spectrum VCSEL
  cmdData[2] = (((uint8_t)_spadDeadTime & 0x07) << 3) |
               (((uint8_t)_spadConfig & 0x03) << 6); // No cal loading
  cmdData[3] = 0x02;                                 // distanceEnabled = 1
  cmdData[4] = 0x00;                                 // GPIO settings
  cmdData[5] = 0x00;                                 // DAX delay
  cmdData[6] = _snrThreshold & 0x3F;                 // SNR threshold
  cmdData[7] = 0x00;               // Single-shot for calibration
  cmdData[8] = 4000 & 0xFF;        // 4000k iterations LSB
  cmdData[9] = (4000 >> 8) & 0xFF; // 4000k iterations MSB
  cmdData[10] = TMF8806_CMD_FACTORY_CALIB;

  // Write command
  uint8_t reg = TMF8806_REG_CMD_DATA9;
  if (!_i2c_dev->write(cmdData, 11, true, &reg, 1)) {
    return false;
  }

  // Wait for command to be accepted
  if (!executeCommand(TMF8806_CMD_FACTORY_CALIB, 5)) {
    return false;
  }

  // Wait for calibration to complete (can take several seconds)
  // Poll interrupt status
  uint32_t startMs = millis();
  while ((millis() - startMs) < 30000) { // 30 second timeout
    if (dataReady()) {
      break;
    }
    delay(100);
  }

  if (!dataReady()) {
    return false;
  }

  // Read calibration data
  return getCalibrationData(_calibData, TMF8806_CALIB_DATA_SIZE);
}

/*!
 * @brief Get calibration data from device
 * @param data Buffer to store calibration data
 * @param len Length of buffer (should be 14)
 * @return true on success, false on failure
 */
bool Adafruit_TMF8806::getCalibrationData(uint8_t* data, uint8_t len) {
  if (len < TMF8806_CALIB_DATA_SIZE) {
    return false;
  }

  // Read CONTENT register + calibration data
  uint8_t regContent = TMF8806_REG_CONTENT;
  uint8_t buffer[2 + TMF8806_CALIB_DATA_SIZE];
  if (!_i2c_dev->write_then_read(&regContent, 1, buffer, sizeof(buffer))) {
    return false;
  }

  // Check content type
  if (buffer[0] != TMF8806_CONTENT_CALIB) {
    return false;
  }

  // Copy calibration data (starts at offset 2)
  memcpy(data, &buffer[2], TMF8806_CALIB_DATA_SIZE);

  // Clear interrupt
  Adafruit_BusIO_Register int_status_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_INT_STATUS);
  Adafruit_BusIO_RegisterBits int1_status =
      Adafruit_BusIO_RegisterBits(&int_status_reg, 1, 0);
  int1_status.write(1);

  return true;
}

/*!
 * @brief Set calibration data
 * @param data Calibration data buffer
 * @param len Length of data (should be 14)
 * @return true on success, false on failure
 */
bool Adafruit_TMF8806::setCalibrationData(const uint8_t* data, uint8_t len) {
  if (len > TMF8806_CALIB_DATA_SIZE) {
    len = TMF8806_CALIB_DATA_SIZE;
  }

  memcpy(_calibData, data, len);
  _hasCalibData = true;

  return true;
}

/*!
 * @brief Enable or disable use of calibration data
 * @param enable true to use calibration, false to disable
 */
void Adafruit_TMF8806::enableCalibration(bool enable) {
  _useCalibration = enable;
}

/*!
 * @brief Get chip ID
 * @return Chip ID (bits [5:0] of register 0xE3)
 */
uint8_t Adafruit_TMF8806::getChipID() {
  Adafruit_BusIO_Register id_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_ID);
  Adafruit_BusIO_RegisterBits chip_id =
      Adafruit_BusIO_RegisterBits(&id_reg, 6, 0);
  return chip_id.read();
}

/*!
 * @brief Get chip revision ID
 * @return Revision ID (bits [2:0] of REVID register)
 */
uint8_t Adafruit_TMF8806::getRevisionID() {
  Adafruit_BusIO_Register revid_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_REVID);
  Adafruit_BusIO_RegisterBits rev_id =
      Adafruit_BusIO_RegisterBits(&revid_reg, 3, 0);
  return rev_id.read();
}

/*!
 * @brief Get last measured temperature
 * @return Temperature in degrees Celsius
 */
int8_t Adafruit_TMF8806::getTemperature() {
  return _lastTemperature;
}

/*!
 * @brief Get firmware version
 * @param major Pointer to store major version
 * @param minor Pointer to store minor version
 * @param patch Pointer to store patch version
 */
void Adafruit_TMF8806::getVersion(uint8_t* major, uint8_t* minor,
                                  uint8_t* patch) {
  Adafruit_BusIO_Register major_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_APPREV_MAJOR);
  Adafruit_BusIO_Register minor_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_APPREV_MINOR);
  Adafruit_BusIO_Register patch_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_APPREV_PATCH);

  *major = major_reg.read();
  *minor = minor_reg.read();
  *patch = patch_reg.read();
}

/*!
 * @brief Read device serial number
 * @param serial Buffer to store serial number (4 bytes)
 * @param len Length of buffer
 * @return true on success, false on failure
 */
bool Adafruit_TMF8806::readSerialNumber(uint8_t* serial, uint8_t len) {
  // Send read serial command
  Adafruit_BusIO_Register command_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_COMMAND);
  if (!command_reg.write(TMF8806_CMD_READ_SERIAL)) {
    return false;
  }

  // Wait for command completion
  if (!executeCommand(TMF8806_CMD_READ_SERIAL, 5)) {
    return false;
  }

  // Read content register + serial number
  // Serial number is at registers 0x28-0x2B (4 bytes)
  // We need to read from 0x1E (CONTENT) to at least 0x2B
  uint8_t regContent = TMF8806_REG_CONTENT;
  uint8_t buffer[14]; // 0x1E to 0x2B = 14 bytes
  if (!_i2c_dev->write_then_read(&regContent, 1, buffer, 14)) {
    return false;
  }

  // Check content type
  if (buffer[0] != TMF8806_CONTENT_SERIAL) {
    return false;
  }

  // Serial number is at offset 10 (0x28 - 0x1E = 10)
  uint8_t copyLen = (len < 4) ? len : 4;
  memcpy(serial, &buffer[10], copyLen);

  return true;
}

// ============================================================================
// Private helper methods
// ============================================================================

/*!
 * @brief Wait for CPU to be ready
 * @param timeoutMs Timeout in milliseconds
 * @return true if CPU ready, false on timeout
 */
/*!
 * @brief Wake device, start measurement app, and enable interrupts
 * @return true on success, false on failure
 */
bool Adafruit_TMF8806::startApp() {
  // Set PON bit to wake up device
  Adafruit_BusIO_Register enable_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_ENABLE);
  Adafruit_BusIO_RegisterBits pon_bit =
      Adafruit_BusIO_RegisterBits(&enable_reg, 1, 0);
  if (!pon_bit.write(1)) {
    return false;
  }

  // Wait for CPU ready
  if (!waitForCpuReady(100)) {
    return false;
  }

  // Request measurement application
  Adafruit_BusIO_Register appreqid_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_APPREQID);
  if (!appreqid_reg.write(TMF8806_APP_MEASUREMENT)) {
    return false;
  }

  delay(10);

  // Wait for app to start
  if (!waitForApp(100)) {
    return false;
  }

  // Enable result interrupt
  Adafruit_BusIO_Register int_status_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_INT_STATUS);
  Adafruit_BusIO_RegisterBits int1_status =
      Adafruit_BusIO_RegisterBits(&int_status_reg, 1, 0);
  if (!int1_status.write(1)) { // Clear by writing 1
    return false;
  }
  Adafruit_BusIO_Register int_enab_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_INT_ENAB);
  Adafruit_BusIO_RegisterBits int1_enab =
      Adafruit_BusIO_RegisterBits(&int_enab_reg, 1, 0);
  if (!int1_enab.write(1)) { // Enable result interrupt
    return false;
  }

  return true;
}

/*!
 * @brief Wait for CPU to be ready
 * @param timeoutMs Timeout in milliseconds
 * @return true if CPU ready, false on timeout
 */
bool Adafruit_TMF8806::waitForCpuReady(uint16_t timeoutMs) {
  Adafruit_BusIO_Register enable_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_ENABLE);
  Adafruit_BusIO_RegisterBits pon_bit =
      Adafruit_BusIO_RegisterBits(&enable_reg, 1, 0);
  Adafruit_BusIO_RegisterBits cpu_ready_bit =
      Adafruit_BusIO_RegisterBits(&enable_reg, 1, 6);
  uint32_t startMs = millis();
  while ((millis() - startMs) < timeoutMs) {
    if (pon_bit.read() && cpu_ready_bit.read()) {
      return true;
    }
    delay(1);
  }
  return false;
}

/*!
 * @brief Wait for application to be running
 * @param timeoutMs Timeout in milliseconds
 * @return true if app running, false on timeout
 */
bool Adafruit_TMF8806::waitForApp(uint16_t timeoutMs) {
  Adafruit_BusIO_Register appid_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_APPID);
  uint32_t startMs = millis();
  while ((millis() - startMs) < timeoutMs) {
    uint8_t appId = appid_reg.read();
    if (appId == TMF8806_APP_MEASUREMENT) {
      return true;
    }
    delay(1);
  }
  return false;
}

/*!
 * @brief Wait for command to complete
 * @param cmd Expected command in PREVIOUS register
 * @param timeoutMs Timeout in milliseconds
 * @return true if command completed, false on timeout/error
 */
bool Adafruit_TMF8806::executeCommand(uint8_t cmd, uint16_t timeoutMs) {
  Adafruit_BusIO_Register state_reg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8806_REG_STATE);
  uint32_t startMs = millis();
  while ((millis() - startMs) < timeoutMs) {
    uint8_t regCmd = TMF8806_REG_COMMAND;
    uint8_t buffer[2];
    // Read COMMAND (0x10) and PREVIOUS (0x11)
    if (!_i2c_dev->write_then_read(&regCmd, 1, buffer, 2)) {
      return false;
    }

    // Success: COMMAND = 0x00 and PREVIOUS = expected command
    if (buffer[0] == 0x00 && buffer[1] == cmd) {
      return true;
    }

    // Check for error state
    uint8_t state = state_reg.read();
    if (state == 0x02) { // Error state
      return false;
    }

    delayMicroseconds(100);
  }
  return false;
}

/*!
 * @brief Get maximum distance for current mode
 * @return Maximum distance in mm
 */
uint16_t Adafruit_TMF8806::getMaxDistance() {
  switch (_distanceMode) {
    case TMF8806_MODE_SHORT_RANGE:
      return TMF8806_MAX_SHORT_RANGE;
    case TMF8806_MODE_5M:
      return TMF8806_MAX_5M;
    case TMF8806_MODE_2_5M:
    default:
      return TMF8806_MAX_2_5M;
  }
}
