# TMF8806 Time-of-Flight Sensor - Design Document

## Chip Overview

| Property | Value |
|----------|-------|
| **Part Number** | TMF8806 |
| **Manufacturer** | ams-OSRAM |
| **Type** | Direct Time-of-Flight (dToF) Distance Sensor |
| **I2C Address** | 0x41 (7-bit, changeable via command 0x49) |
| **Chip ID Register** | 0xE3, bits [5:0] |
| **Expected Chip ID** | 0x09 |
| **Package** | OLGA12 (2.2mm × 3.6mm × 1.0mm) |
| **Supply Voltage** | 2.7V - 3.5V (typ 3.1V) |
| **I/O Voltage** | 1.2V or 1.8V-3.3V (auto-detected at startup) |
| **I2C Speed** | Up to 1 MHz (Fast Mode Plus) |

### Key Features
- SPAD (Single Photon Avalanche Diode) array with TDC and histogram processing
- Maximum detection: 10,000mm (10m mode), 5,000mm (5m mode), 2,500mm (2.5m mode)
- Minimum distance: 10mm (default) to 40mm (thick glass config)
- 1mm distance resolution
- Class 1 eye-safe 940nm VCSEL
- Integrated on-chip distance algorithm
- Ultra-low power mode (14µA @ 0.5Hz)
- EMC spread spectrum options

---

## Register Map

### Common Registers (Always Available)

| Address | Name | R/W | Width | Reset | Description |
|---------|------|-----|-------|-------|-------------|
| 0x00 | APPID | RW | 8 | 0x00 | Running application ID |
| 0x01 | APPREV_MAJOR | RW | 8 | 0x00 | App major revision |
| 0x02 | APPREQID | RW | 8 | 0x00 | Request app to start |
| 0xE0 | ENABLE | RW | 8 | 0x00 | Power/enable control |
| 0xE1 | INT_STATUS | RW | 8 | 0x00 | Interrupt status (write 1 to clear) |
| 0xE2 | INT_ENAB | RW | 8 | 0x00 | Interrupt enable |
| 0xE3 | ID | RO | 8 | 0x09 | Chip ID (use bits [5:0] only) |
| 0xE4 | REVID | RO | 8 | - | Chip revision |

### App0 Registers (appid = 0xC0)

| Address | Name | R/W | Width | Reset | Description |
|---------|------|-----|-------|-------|-------------|
| 0x06 | CMD_DATA9 | W | 8 | 0x00 | Command parameter byte 9 |
| 0x07 | CMD_DATA8 | W | 8 | 0x00 | Command parameter byte 8 |
| 0x08 | CMD_DATA7 | W | 8 | 0x00 | Command parameter byte 7 |
| 0x09 | CMD_DATA6 | W | 8 | 0x00 | Command parameter byte 6 |
| 0x0A | CMD_DATA5 | W | 8 | 0x00 | Command parameter byte 5 |
| 0x0B | CMD_DATA4 | W | 8 | 0x00 | Command parameter byte 4 |
| 0x0C | CMD_DATA3 | W | 8 | 0x00 | Command parameter byte 3 |
| 0x0D | CMD_DATA2 | W | 8 | 0x00 | Command parameter byte 2 |
| 0x0E | CMD_DATA1 | W | 8 | 0x00 | Command parameter byte 1 |
| 0x0F | CMD_DATA0 | W | 8 | 0x00 | Command parameter byte 0 |
| 0x10 | COMMAND | RO | 8 | 0x00 | Command register (0x00 = complete) |
| 0x11 | PREVIOUS | RO | 8 | 0x00 | Previous/current command |
| 0x12 | APPREV_MINOR | RO | 8 | - | App minor revision |
| 0x13 | APPREV_PATCH | RO | 8 | - | App patch level |
| 0x1C | STATE | RO | 8 | - | Operation state |
| 0x1D | STATUS | RO | 8 | - | Operation status |
| 0x1E | REGISTER_CONTENTS | RO | 8 | - | Type of data in 0x20-0xDF |
| 0x1F | TID | RO | 8 | - | Transaction ID (increments) |

### Result Registers (register_contents = 0x55)

| Address | Name | R/W | Width | Reset | Description |
|---------|------|-----|-------|-------|-------------|
| 0x20 | RESULT_NUMBER | RO | 8 | - | Result counter |
| 0x21 | RESULT_INFO | RO | 8 | - | Status and reliability |
| 0x22 | DISTANCE_PEAK_0 | RO | 8 | - | Distance LSB (mm) |
| 0x23 | DISTANCE_PEAK_1 | RO | 8 | - | Distance MSB (mm) |
| 0x24-0x27 | SYS_CLOCK_0-3 | RO | 32 | - | System clock (0.213µs units) |
| 0x28-0x32 | STATE_DATA_0-10 | RO | 88 | - | Algorithm state (11 bytes) |
| 0x33 | TEMPERATURE | RO | 8 | - | Die temp (signed, °C) |
| 0x34-0x37 | REFERENCE_HITS_0-3 | RO | 32 | - | Reference SPAD hit count |
| 0x38-0x3B | OBJECT_HITS_0-3 | RO | 32 | - | Object SPAD hit count |
| 0x3C | XTALK_LSB | RO | 8 | - | Crosstalk value LSB |
| 0x3D | XTALK_MSB | RO | 8 | - | Crosstalk value MSB |

### Bootloader Registers (appid = 0x80)

| Address | Name | R/W | Width | Reset | Description |
|---------|------|-----|-------|-------|-------------|
| 0x08 | BL_CMD_STAT | RW | 8 | - | Bootloader command/status |
| 0x09 | BL_SIZE | RW | 7 | - | Data size (0-128) |
| 0x0A-0x89 | BL_DATA | RW | var | - | Up to 128 bytes payload |
| after data | BL_CSUM | RW | 8 | - | Checksum |

---

## Bit Field Details

### ENABLE Register (0xE0)
```cpp
Adafruit_BusIO_RegisterBits cpu_reset(&enable_reg, 1, 7);  // Write 1 to reset CPU (self-clearing)
Adafruit_BusIO_RegisterBits cpu_ready(&enable_reg, 1, 6);  // RO: CPU ready for I2C
Adafruit_BusIO_RegisterBits pon(&enable_reg, 1, 0);        // Power on: 1=activate oscillator
```

### INT_STATUS Register (0xE1)
```cpp
Adafruit_BusIO_RegisterBits int2(&int_status_reg, 1, 1);   // Raw histogram available (write 1 to clear)
Adafruit_BusIO_RegisterBits int1(&int_status_reg, 1, 0);   // Object detection result (write 1 to clear)
```

### INT_ENAB Register (0xE2)
```cpp
Adafruit_BusIO_RegisterBits int2_enab(&int_enab_reg, 1, 1);  // Enable histogram interrupt
Adafruit_BusIO_RegisterBits int1_enab(&int_enab_reg, 1, 0);  // Enable result interrupt
```

### ID Register (0xE3)
```cpp
Adafruit_BusIO_RegisterBits chip_id(&id_reg, 6, 0);        // Chip ID (should be 0x09)
// Bits 7:6 are reserved - do not rely on them
```

### RESULT_INFO Register (0x21)
```cpp
Adafruit_BusIO_RegisterBits measStatus(&result_info_reg, 2, 6);  // Measurement status
Adafruit_BusIO_RegisterBits reliability(&result_info_reg, 6, 0); // Reliability 0-63
```

### CMD_DATA9 - SpreadSpectrumSpadChargePump (0x06)
```cpp
Adafruit_BusIO_RegisterBits spad_ss_amplitude(&cmd_data9, 4, 0);  // 0-15 (15=max spread)
Adafruit_BusIO_RegisterBits spad_ss_config(&cmd_data9, 2, 4);     // 1=enable, 0=disable
// Bits 7:6 must be 0
```

### CMD_DATA8 - SpreadSpectrumVcselChargePump (0x07)
```cpp
Adafruit_BusIO_RegisterBits vcsel_ss_amplitude(&cmd_data8, 4, 0); // 0-15 (15=max spread)
Adafruit_BusIO_RegisterBits vcsel_ss_config(&cmd_data8, 2, 4);    // 1=enable, 0=disable
// Bits 7:6 must be 0
```

### CMD_DATA7 - Calibration/Config (0x08)
```cpp
Adafruit_BusIO_RegisterBits factoryCal(&cmd_data7, 1, 0);    // 1=factory calib data loaded
Adafruit_BusIO_RegisterBits algState(&cmd_data7, 1, 1);      // 1=algorithm state loaded (requires factoryCal=1)
// Bit 2 must be 0
Adafruit_BusIO_RegisterBits spadDeadTime(&cmd_data7, 3, 3);  // SPAD recovery: 0=97ns, 4=16ns(default), 7=4ns
Adafruit_BusIO_RegisterBits spadSelect(&cmd_data7, 2, 6);    // Optical stack config
```

### CMD_DATA6 - Algorithm Config (0x09)
```cpp
// Bit 0 must be 0
Adafruit_BusIO_RegisterBits distanceEnabled(&cmd_data6, 1, 1);    // 0=short range only, 1=full range
Adafruit_BusIO_RegisterBits vcselClkDiv2(&cmd_data6, 1, 2);       // 1=halve VCSEL freq (for 5m/10m or EMC)
Adafruit_BusIO_RegisterBits distanceMode(&cmd_data6, 1, 3);       // 0=2.5m, 1=5m/10m
Adafruit_BusIO_RegisterBits immediateInterrupt(&cmd_data6, 1, 4); // 1=report GPIO interrupts immediately
// Bit 5 reserved, keep 0
Adafruit_BusIO_RegisterBits distanceMode10m(&cmd_data6, 1, 6);    // 1=10m mode (needs FW download)
Adafruit_BusIO_RegisterBits algKeepReady(&cmd_data6, 1, 7);       // 1=don't standby between measurements
```

### CMD_DATA5 - GPIO Control (0x0A)
```cpp
Adafruit_BusIO_RegisterBits gpio0(&cmd_data5, 4, 0);  // GPIO0 mode (0-9)
Adafruit_BusIO_RegisterBits gpio1(&cmd_data5, 4, 4);  // GPIO1 mode (0-9)
```

### CMD_DATA3 - Threshold/Spread Spectrum (0x0C)
```cpp
Adafruit_BusIO_RegisterBits threshold(&cmd_data3, 6, 0);              // Detection threshold (0=default)
Adafruit_BusIO_RegisterBits vcselClkSpreadSpecAmplitude(&cmd_data3, 2, 6); // 0=off, 1-3=amplitude
```

---

## Typed Enums

### Application IDs
```cpp
typedef enum {
  TMF8806_APP_BOOTLOADER = 0x80,
  TMF8806_APP_MEASUREMENT = 0xC0
} tmf8806_appid_t;
```

### Commands
```cpp
typedef enum {
  TMF8806_CMD_NONE = 0x00,
  TMF8806_CMD_MEASURE = 0x02,
  TMF8806_CMD_WRITE_CONFIG = 0x08,
  TMF8806_CMD_READ_CONFIG = 0x09,
  TMF8806_CMD_FACTORY_CALIB = 0x0A,
  TMF8806_CMD_GPIO_CONTROL = 0x0F,
  TMF8806_CMD_ENABLE_HISTOGRAM = 0x30,
  TMF8806_CMD_CONTINUE_HISTOGRAM = 0x32,
  TMF8806_CMD_READ_SERIAL = 0x47,
  TMF8806_CMD_CHANGE_I2C_ADDR = 0x49,
  TMF8806_CMD_READ_HISTOGRAM = 0x80,
  TMF8806_CMD_STOP = 0xFF
} tmf8806_command_t;
```

### Bootloader Commands
```cpp
typedef enum {
  TMF8806_BL_RAMREMAP_RESET = 0x11,
  TMF8806_BL_DOWNLOAD_INIT = 0x14,
  TMF8806_BL_W_RAM = 0x41,
  TMF8806_BL_ADDR_RAM = 0x43
} tmf8806_bl_command_t;
```

### Distance Modes
```cpp
typedef enum {
  TMF8806_MODE_SHORT_RANGE = 0,  // 200mm max, lowest power
  TMF8806_MODE_2_5M = 1,         // 2500mm max (default)
  TMF8806_MODE_5M = 2,           // 5000mm max
  TMF8806_MODE_10M = 3           // 10000mm max (requires FW download)
} tmf8806_distance_mode_t;
```

### Optical Stack Configurations (spadSelect)
```cpp
typedef enum {
  TMF8806_SPAD_DEFAULT = 0,         // 0.5mm airgap, 0.55mm glass, min 0mm
  TMF8806_SPAD_LARGE_AIRGAP = 1,    // 1mm airgap, min 20mm
  TMF8806_SPAD_THICK_GLASS = 2      // 0mm airgap, 3.2mm glass, min 40mm
} tmf8806_spad_config_t;
```

### SPAD Dead Time Settings (spadDeadTime)
```cpp
typedef enum {
  TMF8806_SPAD_DEADTIME_97NS = 0,   // Best short-range accuracy
  TMF8806_SPAD_DEADTIME_48NS = 1,
  TMF8806_SPAD_DEADTIME_32NS = 2,
  TMF8806_SPAD_DEADTIME_24NS = 3,
  TMF8806_SPAD_DEADTIME_16NS = 4,   // Default - good balance
  TMF8806_SPAD_DEADTIME_12NS = 5,
  TMF8806_SPAD_DEADTIME_8NS = 6,
  TMF8806_SPAD_DEADTIME_4NS = 7     // Best sunlight performance
} tmf8806_spad_deadtime_t;
```

### GPIO Modes
```cpp
typedef enum {
  TMF8806_GPIO_DISABLED = 0,
  TMF8806_GPIO_INPUT_ACTIVE_LOW = 1,   // Low disables measurement
  TMF8806_GPIO_INPUT_ACTIVE_HIGH = 2,  // High disables measurement
  TMF8806_GPIO_OUTPUT_VCSEL_PULSE = 3, // VCSEL timing signal
  TMF8806_GPIO_OUTPUT_LOW = 4,
  TMF8806_GPIO_OUTPUT_HIGH = 5,
  TMF8806_GPIO_OUTPUT_DETECT_HIGH = 6, // High when object detected
  TMF8806_GPIO_OUTPUT_DETECT_LOW = 7,  // Low when object detected
  TMF8806_GPIO_OD_NO_DETECT_LOW = 8,   // Open-drain, low on no-detect
  TMF8806_GPIO_OD_DETECT_LOW = 9       // Open-drain, low on detect
} tmf8806_gpio_mode_t;
```

### Measurement Status (measStatus bits [7:6])
```cpp
typedef enum {
  TMF8806_MEAS_SHORT_INTERRUPTED = 0,  // Short capture interrupted, using previous short only
  TMF8806_MEAS_SHORT_LONG_PREV = 1,    // Short interrupted, using previous short+long
  TMF8806_MEAS_LONG_INTERRUPTED = 2,   // Long interrupted, short result only
  TMF8806_MEAS_COMPLETE = 3            // Complete result (short + long)
} tmf8806_meas_status_t;
```

### Register Contents Types
```cpp
typedef enum {
  TMF8806_CONTENTS_CALIBRATION = 0x0A,
  TMF8806_CONTENTS_SERIAL = 0x47,
  TMF8806_CONTENTS_RESULTS = 0x55,
  TMF8806_CONTENTS_HISTOGRAM_TDC0_BIN0 = 0x80,   // Through 0x93 for all TDCs/bins
  TMF8806_CONTENTS_HISTOGRAM_TDC4_BIN196 = 0x93
} tmf8806_register_contents_t;
```

### Repetition Period Special Values
```cpp
typedef enum {
  TMF8806_PERIOD_SINGLE = 0x00,   // Single measurement
  TMF8806_PERIOD_1000MS = 0xFE,   // 1 second
  TMF8806_PERIOD_2000MS = 0xFF    // 2 seconds
  // Values 1-253 = period in milliseconds
} tmf8806_repetition_period_t;
```

---

## Multi-Byte Read/Write Order

### Little-Endian (LSB First)
All multi-byte values in TMF8806 are **little-endian** (LSB at lower address):

| Value | Byte Order |
|-------|------------|
| Distance (16-bit) | `[0x22]=LSB, [0x23]=MSB` → `distance = reg[0x22] + (reg[0x23] << 8)` |
| System Clock (32-bit) | `[0x24]=byte0, [0x25]=byte1, [0x26]=byte2, [0x27]=byte3` |
| Reference Hits (32-bit) | `[0x34]=LSB ... [0x37]=MSB` |
| Object Hits (32-bit) | `[0x38]=LSB ... [0x3B]=MSB` |
| Crosstalk (16-bit) | `[0x3C]=LSB, [0x3D]=MSB` |
| Iterations (16-bit) | `cmd_data1=LSB (kIters), cmd_data0=MSB (kIters*256)` |
| Thresholds (16-bit) | `_LSB + 256 * _MSB` |

### System Clock Note
**The sys_clock value is only valid when its LSB bit is 1.** Always perform a block read from 0x1D to 0x27 to ensure proper synchronization.

---

## Timing Requirements

### Startup Timing
| Phase | Time | Notes |
|-------|------|-------|
| Power-on to I2C ready | 1.6ms | After EN goes high |
| Boot to app ready | ~5ms | From ROM boot |
| Standby to active | <1ms | |
| Active to standby | <1ms | |
| Enable low to power down | <1ms | |
| Ranging init (first/temp change) | 8ms | Electrical calibration |

### Measurement Timing
| Mode | Typical Period | Iterations | Notes |
|------|----------------|------------|-------|
| 2.5m default (33ms) | 33ms | 900k | Default mode |
| 5m (66ms) | 66ms | 900k | Higher accuracy |
| 5m (33ms) | 33ms | 450k | Lower accuracy |
| 10m | variable | 1500k | Requires FW download |
| Ultra-low power | 1000-2000ms | 10k | Lowest current |

### Iteration Ranges
- **Minimum**: 10k iterations
- **Default**: 900k iterations  
- **Maximum**: 4000k (4M) iterations
- **Calibration**: 40M iterations recommended

---

## Boot Sequence

### Power-On / Enable Sequence
```
1. Apply VDD (2.7-3.5V) to all supply pins
2. Set EN pin HIGH (or tie to VDD)
3. Wait 1.6ms for I2C ready
4. Poll ENABLE[0xE0] until == 0x00 (bootloader sleep)
5. Set ENABLE.pon = 1 (write 0x01 to 0xE0)
6. Poll ENABLE[0xE0] until == 0x41 (bootloader ready)
7. Write APPREQID[0x02] = 0xC0 to start measurement app
8. Poll APPID[0x00] until == 0xC0 (app running)
9. Enable interrupts: INT_ENAB[0xE2] = 0x01
10. Load factory calibration to 0x20-0x2D (if available)
11. Start measurement with command 0x02
```

### I/O Voltage Auto-Detection
At startup (EN rising edge), GPIO0 voltage determines I/O level:
- **GPIO0 < 1.5V**: 1.2V I/O mode selected
- **GPIO0 ≥ 1.5V**: 1.8V-3.3V I/O mode selected

**Tip**: For 1.8V-3.3V operation, leave GPIO0 floating or connect to VDD.
For 1.2V operation, connect GPIO0 to GND during startup.

---

## Calibration

### Factory Calibration Requirements
- Must be performed **per distance mode** (short range + 2.5m share calibration)
- Must be performed **per optical stack configuration**
- Environment: minimal ambient light, no target within 40cm in FoV
- Iterations: 40M for best results
- Result: 14 bytes stored at 0x20-0x2D

### Calibration Data Format (14 bytes)
```cpp
// Byte 0: Format revision
//   Bits [3:0] = revision (current = 0x2)
//   Bits [7:4] = reserved
// Bytes 1-13: Calibration parameters (opaque)
```

### Loading Calibration
Before starting measurement (command 0x02), write calibration data to 0x20-0x2D and set `factoryCal=1` in CMD_DATA7.

### Algorithm State (Ultra-Low Power Mode)
For power cycling between measurements:
1. After measurement, read STATE_DATA (0x28-0x32, 11 bytes)
2. Power down (EN=0)
3. On next power-up, write state to STATE_DATA_WR (0x2E-0x38)
4. Set `algState=1` AND `factoryCal=1` when starting measurement

---

## Command Protocol

### Command Execution Flow
```
1. Write CMD_DATA9-0 (0x06-0x0F) with parameters
2. Write COMMAND (0x10) with command byte
3. Poll COMMAND until == 0x00 (complete)
4. Check STATUS (0x1D) for errors (0x00-0x0F = OK)
5. Previous command stored in PREVIOUS (0x11)
```

### Start Measurement (Command 0x02)
Write to registers 0x06-0x0F then 0x10:

| Register | Name | Default | Purpose |
|----------|------|---------|---------|
| 0x06 | SpreadSpecSPAD | 0x00 | SPAD charge pump EMC |
| 0x07 | SpreadSpecVCSEL | 0x00 | VCSEL charge pump EMC |
| 0x08 | CalibConfig | 0x11 | Calib flags + SPAD settings |
| 0x09 | AlgConfig | 0x02 | Distance mode + flags |
| 0x0A | GPIOConfig | 0x00 | GPIO0/GPIO1 modes |
| 0x0B | ALSDelay | 0x00 | ALS sync delay (100µs units) |
| 0x0C | Threshold | 0x06 | Detection threshold |
| 0x0D | RepPeriod | 0x1E | Period (30ms) or special |
| 0x0E | kIters_L | 0x84 | Iterations LSB (132 = 132k... wait) |
| 0x0F | kIters_H | 0x03 | Iterations MSB |

**Example**: 900k iterations = 0x0384 → CMD_DATA1=0x84, CMD_DATA0=0x03

### Reading Results
1. Wait for INT pin low (or poll INT_STATUS.int1)
2. Block read from 0x1D to at least 0x27 (or 0x3D for all data)
3. Clear interrupt: write 0x01 to INT_STATUS (0xE1)

---

## Function Map (Proposed Public API)

### Initialization
```cpp
bool begin(TwoWire *wire = &Wire, uint8_t addr = 0x41);
bool reset();                      // Toggle EN or software reset
uint8_t getChipID();               // Should return 0x09
void getVersion(uint8_t *major, uint8_t *minor, uint8_t *patch);
```

### Configuration
```cpp
void setDistanceMode(tmf8806_distance_mode_t mode);
void setOpticalConfig(tmf8806_spad_config_t config);
void setSpadDeadTime(tmf8806_spad_deadtime_t deadtime);
void setIterations(uint16_t kIterations);  // In units of 1000
void setRepetitionPeriod(uint8_t periodMs);  // Or special values
void setThreshold(uint8_t threshold);
void setGPIOMode(uint8_t gpioNum, tmf8806_gpio_mode_t mode);
void enableSpreadSpectrum(bool spad, bool vcsel, bool vcselClk);
```

### Calibration
```cpp
bool performFactoryCalibration();
bool loadCalibration(const uint8_t *calibData, size_t len = 14);
bool saveCalibration(uint8_t *calibData, size_t len = 14);
bool loadAlgorithmState(const uint8_t *stateData, size_t len = 11);
bool saveAlgorithmState(uint8_t *stateData, size_t len = 11);
```

### Measurement
```cpp
bool startMeasurement(bool continuous = true);
bool stopMeasurement();
bool dataReady();                  // Check interrupt
bool readResult(tmf8806_result_t *result);
int16_t readDistance();            // Simple distance read (mm)
```

### Result Structure
```cpp
typedef struct {
  uint8_t result_num;
  uint8_t reliability;       // 0-63, 63=best, 0=no object
  tmf8806_meas_status_t status;
  uint16_t distance_mm;
  uint32_t sys_clock;
  int8_t temperature;        // Die temperature in °C
  uint32_t reference_hits;
  uint32_t object_hits;
  uint16_t crosstalk;
} tmf8806_result_t;
```

### Power Management
```cpp
void powerDown();                  // EN low or PON=0
void powerUp();
void setUltraLowPowerMode(bool enable);
```

### Advanced
```cpp
bool changeI2CAddress(uint8_t newAddr);
bool readSerialNumber(uint8_t *serial, uint8_t *id);
bool enableHistogramOutput(uint8_t mask);
bool readHistogram(uint8_t *buffer, size_t len);
```

---

## Quirks and Gotchas

### ⚠️ Critical Notes

1. **Bootloader vs App Mode**: Device boots to bootloader (0x80), must request app (0xC0) explicitly. Many registers only valid in app mode.

2. **Command Register is Write-Only for Execution**: Write command byte, then poll until 0x00. The register itself doesn't store the command after execution.

3. **Register 0x20-0xDF is Multiplexed**: Contents depend on context:
   - After measurement: results (0x55)
   - After calibration command: calibration data (0x0A)  
   - After serial command: serial number (0x47)
   - After histogram command: histogram data (0x80-0x93)

4. **Chip ID Register**: Only bits [5:0] of 0xE3 are valid. Bits [7:6] are undefined.

5. **I/O Voltage Detection**: Happens ONCE at startup (EN edge). GPIO0 must be at correct level during EN rise. After boot, GPIO0 can be used normally.

6. **Calibration is Mode-Specific**: Each distance mode + optical config combination needs separate calibration data. Short range and 2.5m can share calibration.

7. **10m Mode Requires Firmware Download**: Internal ROM only supports up to 5m. For 10m, must download firmware via bootloader.

8. **System Clock Validity**: sys_clock is only valid when LSB bit is 1. Always do block read from 0x1D-0x27.

9. **Algorithm State for Ultra-Low Power**: When power cycling between measurements, must save/restore algorithm state (11 bytes) for continuity.

10. **Spread Spectrum for EMC**: For 2.5m mode with vcselClkSpreadSpecAmplitude, set vcselClkDiv2=1 or max distance is reduced.

11. **Interrupt Clearing**: Write 0x01 to INT_STATUS (0xE1) to clear int1. The bit is write-1-to-clear.

12. **I2C Address Reset**: Setting EN=0 resets I2C address to default 0x41. Must reprogram after each power cycle.

13. **No Protection Diodes on Some Pins**: SDA, SCL, INT, EN have no diode to VDD. They won't block the bus if VDD=0, but GPIO0/GPIO1 have protection diodes and shouldn't be driven externally if VDD is off.

14. **Reliability Values**:
    - 0 = no object detected
    - 1 = short range result, device NOT calibrated
    - 10 = short range result, device IS calibrated
    - Other = long range algorithm result

15. **Crosstalk Register**: Only valid with minimal ambient light and no target within 40cm in FoV.

### Performance Notes

- **Sunlight**: Use shorter SPAD dead time (spadDeadTime=7) for better sunlight performance at cost of short-range accuracy
- **Short Range Accuracy**: Use longer SPAD dead time (spadDeadTime=0) for better short-range accuracy
- **Default**: spadDeadTime=4 (16ns) is good balance

### Differences from TMF8801/TMF8805
- TMF8806 has EMC spread spectrum options
- TMF8806 supports up to 10m (with firmware)
- TMF8806 has ultra-low power mode
- TMF8806 uses different register layout for some features
- TMF8806 I/O voltage auto-detection via GPIO0
