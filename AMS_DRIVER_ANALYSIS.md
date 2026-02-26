# AMS TMF8806 Arduino Driver Analysis

## Overview

This document analyzes the official ams-OSRAM TMF8806 Arduino driver (v2.12) to inform our Adafruit BusIO-based library implementation.

**Source files analyzed:**
- `tmf8806_shim.h/cpp` — Platform abstraction layer
- `tmf8806.h/c` — Core driver (portable C)
- `tmf8806_regs.h` — Register definitions & data structures
- `tmf8806_app.h/cpp` — Example console application
- `tmf8806_image.h/c` — Firmware patch for 10m mode

---

## 1. Architecture Overview

### File Relationships

```
┌─────────────────────────────────────────────────────────────┐
│                    tmf8806_app.cpp                          │
│              (Example Arduino Application)                  │
└─────────────────────┬───────────────────────────────────────┘
                      │ uses
┌─────────────────────▼───────────────────────────────────────┐
│                      tmf8806.c                              │
│           (Core Driver - Portable C Code)                   │
│  • Boot sequence       • Command execution                  │
│  • Measurement control • Calibration                        │
│  • Result reading      • Clock correction                   │
└─────────────────────┬───────────────────────────────────────┘
                      │ uses
┌─────────────────────▼───────────────────────────────────────┐
│                   tmf8806_shim.cpp                          │
│         (Platform Abstraction - Arduino Specific)           │
│  • I2C transactions    • GPIO control                       │
│  • Timing functions    • Serial I/O                         │
└─────────────────────┬───────────────────────────────────────┘
                      │ includes
┌─────────────────────▼───────────────────────────────────────┐
│                   tmf8806_regs.h                            │
│              (Data Structures & Constants)                  │
│  • Result frame struct  • Measure command union             │
│  • Factory calib struct • State data struct                 │
└─────────────────────────────────────────────────────────────┘
```

### Design Pattern

The AMS driver uses a **shim pattern** for portability:
- Core driver (`tmf8806.c`) contains all device logic in platform-agnostic C
- Shim layer (`tmf8806_shim.cpp`) provides platform-specific implementations
- Driver struct (`tmf8806Driver`) holds all state, passed as first param to functions

**Key insight:** The shim pattern is over-engineered for our use case. With Adafruit_BusIO, we can directly use `Adafruit_I2CDevice` and eliminate the shim entirely.

---

## 2. Boot Sequence

### Exact Steps from Power-On to Measurement

```
1. Apply VDD (2.7-3.5V)
   └── tmf8806Disable() — ensure EN pin LOW

2. Wait for capacitor discharge
   └── CAP_DISCHARGE_TIME_MS = 3ms

3. Set EN pin HIGH
   └── tmf8806Enable()

4. Wait for enable time
   └── ENABLE_TIME_MS = 2ms

5. Wake up device (set PON bit)
   └── tmf8806Wakeup()
   └── Write 0x01 to ENABLE register (0xE0)

6. Wait for CPU ready
   └── tmf8806IsCpuReady() — CPU_READY_TIME_MS = 2ms
   └── Poll ENABLE register until (reg & 0x41) == 0x41
       (PON=1 and CPU_READY=1)

7. Switch to measurement application
   └── Option A: tmf8806SwitchToRomApplication()
       - Write 0xC0 to register 0x02 (APPREQID)
       - Wait 10ms for version publish
       - Poll APPID (0x00) until == 0xC0
   
   └── Option B: tmf8806DownloadFirmware() [for 10m mode]
       - Bootloader download init (cmd 0x14)
       - Set RAM address (cmd 0x43)
       - Write RAM in 128-byte chunks (cmd 0x41)
       - RAM remap and start (cmd 0x11)

8. Read device info
   └── tmf8806ReadDeviceInfo()
   └── Reads chip ID, revision, app version, serial number

9. Configure device
   └── tmf8806SetConfiguration()
   └── Optionally load factory calibration

10. Enable interrupts
    └── tmf8806ClrAndEnableInterrupts()
    └── Clear then enable bit 0 of INT_ENAB (0xE2)

11. Start measurement
    └── tmf8806StartMeasurement()
```

### Critical Register Writes During Boot

| Step | Register | Value | Purpose |
|------|----------|-------|---------|
| Wake | 0xE0 (ENABLE) | 0x01 | Set PON bit |
| Poll | 0xE0 (ENABLE) | read | Wait for 0x41 (CPU_READY + PON) |
| App Switch | 0x02 (APPREQID) | 0xC0 | Request measurement app |
| Poll | 0x00 (APPID) | read | Wait for 0xC0 |
| Int Enable | 0xE1 (INT_STATUS) | 0x01 | Clear interrupt |
| Int Enable | 0xE2 (INT_ENAB) | 0x01 | Enable result interrupt |

### Timeout Values from Driver

```c
#define CAP_DISCHARGE_TIME_MS       3     // Capacitor discharge
#define ENABLE_TIME_MS              2     // After EN high
#define CPU_READY_TIME_MS           2     // Wait for CPU ready
#define APP_PUBLISH_VERSION_WAIT_TIME_MS 10  // After RAM remap
#define BL_WAKEUP_DELAY_US          200   // Bootloader wakeup
```

---

## 3. Command Protocol

### Command Execution Flow

```
┌─────────────────────────────────────────────────────────┐
│ 1. Write CMD_DATA9-0 (registers 0x06-0x0F)              │
│    └── Contains configuration parameters                │
├─────────────────────────────────────────────────────────┤
│ 2. Write COMMAND register (0x10) with command code      │
│    └── This triggers command execution                  │
├─────────────────────────────────────────────────────────┤
│ 3. Poll COMMAND register until == 0x00                  │
│    └── Also check STATE register for errors             │
│    └── STATE == 0x02 indicates error                    │
├─────────────────────────────────────────────────────────┤
│ 4. Check PREVIOUS register (0x11)                       │
│    └── Should match the command just executed           │
├─────────────────────────────────────────────────────────┤
│ 5. Result/data in registers 0x20+ (depends on command)  │
│    └── CONTENT register (0x1E) indicates data type      │
└─────────────────────────────────────────────────────────┘
```

### Command Timeout Calculation

```c
#define APP_CMD_MEASURE_TIMEOUT_MS        5
#define APP_CMD_FACTORY_CALIB_TIMEOUT_MS  5
#define APP_CMD_CLEARED_TIMEOUT_MS        1
#define APP_CMD_READ_HISTOGRAM_TIMEOUT_MS 3

// Stop command is special - depends on iterations
#define TMF8806_MIN_VCSEL_CLK_FREQ_MHZ    18
#define APP_CMD_STOP_TIMEOUT_MS(iterations) (5 + (iterations)/(TMF8806_MIN_VCSEL_CLK_FREQ_MHZ))
```

### Implementation (from tmf8806AppCheckCommandDone)

```c
static int8_t tmf8806AppCheckCommandDone(tmf8806Driver *driver, 
                                          uint8_t expected, 
                                          uint16_t timeoutInMs) {
  uint32_t timeout100Us = timeoutInMs * 10;
  do {
    // Read CMD and PREV registers together
    i2cRxReg(driver, addr, 0x10, 2, buf);
    
    // Success: COMMAND=0x00 and PREVIOUS=expected
    if (buf[1] == expected && buf[0] == 0x00) {
      return APP_SUCCESS_OK;
    }
    
    // Check STATE register for error
    i2cRxReg(driver, addr, 0x1C, 1, &state);
    if (state == 0x02) {  // TMF8806_APP_STATE_ERROR
      return APP_ERROR_CMD;  // Fast exit on error
    }
    
    delayMicroseconds(100);
  } while (timeout100Us--);
  
  return APP_ERROR_TIMEOUT;
}
```

### Command Codes Used

| Command | Code | Purpose |
|---------|------|---------|
| MEASURE | 0x02 | Start measurement |
| WRITE_CONFIG | 0x08 | Write additional config |
| READ_CONFIG | 0x09 | Read additional config |
| FACTORY_CALIB | 0x0A | Start factory calibration |
| READ_SERIAL | 0x47 | Read serial number |
| CHANGE_I2C_ADDR | 0x49 | Change I2C address |
| HISTOGRAM_READOUT | 0x30 | Configure histogram dump |
| CONTINUE | 0x32 | Continue histogram read |
| READ_HISTOGRAM | 0x80 | Read histogram data |
| REMOTE_CONTROL | 0xC0 | IR remote control mode |
| STOP | 0xFF | Stop measurement |

---

## 4. Firmware Image

### What tmf8806_image Contains

- **Size:** 0xB38 bytes (2872 bytes)
- **Load Address:** 0x00200000 (RAM)
- **Format:** Raw binary data in PROGMEM array
- **Purpose:** Firmware patch for 10m mode and remote control features

### When It's Loaded

The firmware is loaded via bootloader when:
1. `USE_ROM_FIRMWARE` is **NOT** defined
2. Device is in bootloader mode (APPID = 0x80)
3. 10m mode or remote control mode is required

### Is It Required?

**NO** — for basic operation up to 5m mode.

The ROM firmware (built into the chip) supports:
- 2.5m mode
- 5m mode  
- Factory calibration
- All standard measurement features

The RAM patch only adds:
- 10m mode (`algo.reserved = 2`)
- Remote control mode (IR LED control via GPIO1)

**Our recommendation:** Start with ROM-only support. Add firmware download as an advanced optional feature.

### Download Process

```c
int8_t tmf8806DownloadFirmware(driver, imageStartAddress, image, imageSizeInBytes) {
  // 1. Initialize download
  // Command 0x14 with seed 0x29
  
  // 2. Set RAM address  
  // Command 0x43 with 16-bit address
  
  // 3. Write RAM in 128-byte chunks
  // Command 0x41 with data + checksum
  
  // 4. RAM remap (start execution)
  // Command 0x11
  
  // 5. Verify app started
  // Poll APPID for 0xC0
}
```

---

## 5. Measurement Flow

### Start Measurement

```c
int8_t tmf8806StartMeasurement(tmf8806Driver *driver) {
  return tmf8806ConfigureAndExecute(driver, 
                                     TMF8806_COM_CMD_STAT__cmd_measure,
                                     driver->measureConfig.data.kIters,
                                     APP_CMD_MEASURE_TIMEOUT_MS);
}

static int8_t tmf8806ConfigureAndExecute(driver, cmd, kIter, timeout) {
  uint8_t *ptr = driver->dataBuffer;
  uint8_t addCfgSize = 0;
  
  // 1. If factory calibration is enabled, serialize it first
  if (driver->measureConfig.data.data.factoryCal) {
    tmf8806SerializeFactoryCalibration(&driver->factoryCalib, ptr);
    ptr += sizeof(tmf8806FactoryCalibData);  // 14 bytes
    addCfgSize += 14;
  }
  
  // 2. If algorithm state is enabled, serialize it
  if (driver->measureConfig.data.data.algState) {
    tmf8806SerializeStateData(&driver->stateData, ptr);
    addCfgSize += 11;  // sizeof(tmf8806StateData)
  }
  
  // 3. Write additional config to 0x20 if any
  if (addCfgSize) {
    i2cTxReg(driver, addr, 0x20, addCfgSize, driver->dataBuffer);
  }
  
  // 4. Serialize measurement config
  tmf8806SerializeConfiguration(&driver->measureConfig, driver->dataBuffer);
  
  // 5. Write CMD_DATA9-0 and COMMAND (11 bytes at 0x06-0x10)
  i2cTxReg(driver, addr, 0x06, 11, driver->dataBuffer);
  
  // 6. Wait for command completion
  return tmf8806AppCheckCommandDone(driver, cmd, timeout);
}
```

### Read Results

```c
int8_t tmf8806ReadResult(tmf8806Driver *driver, tmf8806DistanceResultFrame *result) {
  uint32_t hTick = getSysTick();  // Host timestamp for clock correction
  
  // Read from STATE (0x1C) through XTALK_MSB (0x3D) = 34 bytes
  i2cRxReg(driver, addr, 0x1C, 34, driver->dataBuffer);
  
  // Check CONTENT register for result type
  if (driver->dataBuffer[2] == 0x55) {  // TMF8806_COM_RESULT__measurement_result
    // Get device timestamp for clock correction
    uint32_t tTick = tmf8806GetUint32(&driver->dataBuffer[8]);  // SYS_TICK at offset 8
    tmf8806ClockCorrectionAddPair(driver, hTick, tTick);
    
    // Deserialize result
    tmf8806DeserializeResultFrame(&driver->dataBuffer[4], result);
    
    // Update driver's state data copy
    tmf8806DeserializeStateData(result->stateData, &driver->stateData);
    
    // Clamp distance if exceeds maximum for mode
    if (result->distPeak > driver->maximumDistance) {
      result->distPeak = 0;
      result->reliability = 0;
    }
    
    return APP_SUCCESS_OK;
  }
  return APP_ERROR_NO_RESULT_PAGE;
}
```

### Interrupt Handling

```c
// Clear and enable specific interrupts
void tmf8806ClrAndEnableInterrupts(tmf8806Driver *driver, uint8_t mask) {
  // Clear by writing 1s to INT_STATUS (0xE1)
  driver->dataBuffer[0] = mask;
  i2cTxReg(driver, addr, 0xE1, 1, driver->dataBuffer);
  
  // Read current enabled interrupts
  i2cRxReg(driver, addr, 0xE2, 1, driver->dataBuffer);
  
  // Enable requested interrupts
  driver->dataBuffer[0] |= mask;
  i2cTxReg(driver, addr, 0xE2, 1, driver->dataBuffer);
}

// Get and clear set interrupts
uint8_t tmf8806GetAndClrInterrupts(tmf8806Driver *driver, uint8_t mask) {
  i2cRxReg(driver, addr, 0xE1, 1, driver->dataBuffer);
  uint8_t setInterrupts = driver->dataBuffer[0] & mask;
  if (setInterrupts) {
    driver->dataBuffer[0] = setInterrupts;  // Clear only those that were set
    i2cTxReg(driver, addr, 0xE1, 1, driver->dataBuffer);
  }
  return setInterrupts;
}
```

### Continuous vs Single-Shot

The driver doesn't distinguish at a protocol level—it's controlled by `repetitionPeriodMs`:

```c
// In measureConfig:
.repetitionPeriodMs = 33  // Continuous at ~30Hz
.repetitionPeriodMs = 0   // Single-shot

// In loopFn (application):
if (tmf8806.measureConfig.data.repetitionPeriodMs == 0) {
  // Single shot - transition to stopped state after result
  tmf8806DisableAndClrInterrupts(&tmf8806, ...);
  stateTmf8806 = TMF8806_STATE_STOPPED;
}
```

---

## 6. Calibration

### Factory Calibration Procedure

```c
int8_t tmf8806FactoryCalibration(tmf8806Driver *driver, uint16_t kIters) {
  // Temporarily disable loading of calibration data
  uint8_t loadFc = driver->measureConfig.data.data.factoryCal;
  uint8_t loadState = driver->measureConfig.data.data.algState;
  driver->measureConfig.data.data.factoryCal = 0;
  driver->measureConfig.data.data.algState = 0;
  
  // Execute calibration command
  res = tmf8806ConfigureAndExecute(driver, 
                                    0x0A,  // FACTORY_CALIB command
                                    kIters, 
                                    APP_CMD_FACTORY_CALIB_TIMEOUT_MS);
  
  // Restore settings
  driver->measureConfig.data.data.factoryCal = loadFc;
  driver->measureConfig.data.data.algState = loadState;
  
  return res;
}

// After interrupt signals completion:
int8_t tmf8806ReadFactoryCalibration(tmf8806Driver *driver) {
  // Read CONTENT + factory calib data
  i2cRxReg(driver, addr, 0x1E, 2 + 14, driver->dataBuffer);
  
  if (driver->dataBuffer[0] == 0x0A) {  // FACTORY_CALIB content type
    tmf8806DeserializeFactoryCalibration(&driver->dataBuffer[2], 
                                          &driver->factoryCalib);
    return APP_SUCCESS_OK;
  }
  return APP_ERROR_NO_FACTORY_CALIB_PAGE;
}
```

### Calibration Data Format (14 bytes)

```c
struct _tmf8806FactoryCalibData {
  uint32_t id:4;                              // Must be 0x2
  uint32_t crosstalkIntensity:20;             // Crosstalk intensity value
  uint32_t crosstalkTdc1Ch0BinPosUQ6Lsb:8;    // First crosstalk bin position
  // ... more packed bitfields for TDC calibration ...
  uint8_t opticalOffsetQ3;                    // Optical system offset
};
```

### Save/Restore Calibration

The driver stores calibration in the `tmf8806Driver.factoryCalib` member. To save/restore:

```c
// Save: serialize to bytes
tmf8806SerializeFactoryCalibration(&driver->factoryCalib, buffer);
// Store buffer[0..13] to EEPROM/flash

// Restore: deserialize from bytes  
tmf8806DeserializeFactoryCalibration(buffer, &driver->factoryCalib);
driver->measureConfig.data.data.factoryCal = 1;  // Enable loading
```

### Algorithm State (Ultra-Low Power Mode)

For power cycling between measurements, save/restore the 11-byte state data:

```c
// After measurement, state data is updated from result frame
// Save to persistent storage
tmf8806SerializeStateData(&driver->stateData, buffer);

// On next power-up, restore
tmf8806DeserializeStateData(buffer, &driver->stateData);
driver->measureConfig.data.data.algState = 1;  // Enable loading
```

### State Data Format (11 bytes)

```c
struct _tmf8806StateData {
  uint8_t id:4;                    // Must be 0x2
  uint8_t reserved0:4;
  uint8_t breakDownVoltage;        // Last selected breakdown voltage
  uint16_t avgRefBinPosUQ9;        // Avg optical reference bin (UQ7.9)
  int8_t calTemp;                  // Last BDV calibration temp
  int8_t force20MhzVcselTemp;      // Temp for 20MHz VCSEL fallback
  int8_t tdcBinCalibrationQ9[5];   // TDC bin width calibration
};
```

---

## 7. Configuration

### MeasureCmd Structure (11 bytes)

The `tmf8806MeasureCmd` is a union allowing access as raw bytes or named bitfields:

```c
union _tmf8806MeasureCmd {
  struct _tmf8806MeasureCmdRaw {
    uint8_t cmdData9;   // 0x06 - Spread spectrum SPAD
    uint8_t cmdData8;   // 0x07 - Spread spectrum VCSEL
    uint8_t cmdData7;   // 0x08 - Calib/dead-time/SPAD select
    uint8_t cmdData6;   // 0x09 - Algorithm config
    uint8_t cmdData5;   // 0x0A - GPIO config
    uint8_t cmdData4;   // 0x0B - DAX delay
    uint8_t cmdData3;   // 0x0C - SNR threshold + spread spec
    uint8_t cmdData2;   // 0x0D - Repetition period
    uint8_t cmdData1;   // 0x0E - kIters LSB
    uint8_t cmdData0;   // 0x0F - kIters MSB
    uint8_t command;    // 0x10 - Command code
  } packed;
  
  struct _tmf8806MeasureCmdConfig {
    // Named bitfields for each setting
    // See tmf8806_regs.h for full definition
  } data;
};
```

### Default Configuration

```c
const tmf8806MeasureCmd defaultConfig = {
  .data = {
    .spreadSpecSpadChp = { .amplitude = 0, .config = 0 },
    .spreadSpecVcselChp = { .amplitude = 0, .config = 0 },
    .data = {
      .factoryCal = 1,      // Load factory calibration
      .algState = 1,        // Load algorithm state
      .spadDeadTime = 0,    // 97ns (best short-range accuracy)
      .spadSelect = 0,      // All SPADs for proximity
    },
    .algo = {
      .distanceEnabled = 1, // Full range (prox + distance)
      .vcselClkDiv2 = 0,    // 37.6MHz VCSEL
      .distanceMode = 0,    // 2.5m mode
      .immediateInterrupt = 0,
      .algKeepReady = 0,    // Power saving between measurements
    },
    .gpio = { .gpio0 = 0, .gpio1 = 0 },
    .daxDelay100us = 0,
    .snr = { .threshold = 6, .vcselClkSpreadSpecAmplitude = 0 },
    .repetitionPeriodMs = 33,  // ~30Hz continuous
    .kIters = 400,             // 400k iterations
    .command = 0x02,           // MEASURE command
  }
};
```

### How SPAD Maps Work

The `spadSelect` field controls which SPADs are used:
- 0 = All SPADs (default, best for normal operation)
- 1 = 40 best SPADs (for high crosstalk situations)
- 2 = 20 best SPADs (extreme crosstalk)
- 3 = Attenuated mode

### How Iterations Are Set

```c
// kIters is in units of 1000 (kilo-iterations)
// Value range: 10 to 4000 (10k to 4M iterations)
// Default: 400 (400k iterations)

// In the config struct:
.kIters = 400  // 400,000 iterations

// Factory calibration uses high iterations:
#define TMF8806_FACTORY_CALIB_KITERS 4000  // 4,000,000 iterations
```

### How Thresholds Are Set

Thresholds use a separate command (0x08/0x09):

```c
int8_t tmf8806SetThresholds(driver, persistence, lowThreshold, highThreshold) {
  driver->dataBuffer[0] = persistence;
  tmf8806SetUint16(lowThreshold, &driver->dataBuffer[1]);
  tmf8806SetUint16(highThreshold, &driver->dataBuffer[3]);
  driver->dataBuffer[5] = 0x08;  // WRITE_CONFIG command
  
  i2cTxReg(driver, addr, 0x0B, 6, driver->dataBuffer);  // CMD_DATA4 through CMD
  return tmf8806AppCheckCommandDone(driver, 0x08, timeout);
}
```

---

## 8. Key Data Structures

### Driver State Structure

```c
typedef struct _tmf8806Driver {
  // Device information
  tmf8806DeviceInfo device;              // Serial number, versions
  tmf8806DriverInfo info;                // Driver version
  
  // Configuration
  tmf8806MeasureCmd measureConfig;       // Current measurement config
  tmf8806FactoryCalibData factoryCalib;  // Current factory calibration
  tmf8806StateData stateData;            // Current algorithm state
  
  // Clock correction
  uint32_t hostTicks[4];                 // Host timestamps
  uint32_t tmf8806Ticks[4];              // Device timestamps  
  uint16_t clkCorrRatioUQ;               // UQ1.15 ratio
  uint8_t clkCorrectionIdx;              // Ring buffer index
  uint8_t clkCorrectionEnable;           // On/off flag
  
  // Device state
  uint8_t i2cSlaveAddress;               // Current I2C address
  uint8_t logLevel;                      // Debug verbosity
  uint16_t maximumDistance;              // Max distance for current mode
  
  // I/O buffer
  uint8_t dataBuffer[256];               // Scratch buffer for I2C
} tmf8806Driver;
```

### Result Frame Structure

```c
typedef struct _tmf8806DistanceResultFrame {
  uint8_t resultNum;           // Result counter (increments)
  uint8_t reliability:6;       // 0-63 (0=no object, 63=best)
  uint8_t resultStatus:2;      // Measurement completion status
  uint16_t distPeak;           // Distance in mm
  uint32_t sysClock;           // Device timestamp (0.2µs units)
  uint8_t stateData[11];       // Algorithm state for save/restore
  int8_t temperature;          // Die temperature in °C
  uint32_t referenceHits;      // Reference SPAD count
  uint32_t objectHits;         // Object SPAD count
  uint16_t xtalk;              // Crosstalk peak value
} tmf8806DistanceResultFrame;
```

### Result Status Values

```c
// resultStatus field interpretation:
// 0 = Short capture interrupted, using previous short only
// 1 = Short interrupted, using previous short+long
// 2 = Long interrupted, short result only  
// 3 = Complete result (short + long) ← Normal successful measurement
```

### Reliability Interpretation

```c
// reliability field (0-63):
// 0   = No object detected
// 1   = Short range result, device NOT calibrated
// 10  = Short range result, device IS calibrated
// 11+ = Long range algorithm result (higher = more confident)
// 63  = Maximum confidence
```

---

## 9. I2C Transaction Patterns

### Block Read/Write

The AMS driver splits transactions into 32-byte chunks (Arduino Wire limitation):

```c
#define ARDUINO_MAX_I2C_TRANSFER 32

static int8_t i2cTxOnly(logLevel, slaveAddr, regAddr, toTx, txData) {
  int8_t res = I2C_SUCCESS;
  do {
    uint8_t tx = (toTx > 31) ? 31 : toTx;  // Max 31 data bytes
    
    Wire.beginTransmission(slaveAddr);
    Wire.write(regAddr);      // Register address first
    if (tx) Wire.write(txData, tx);
    
    toTx -= tx;
    txData += tx;
    regAddr += tx;            // Auto-increment address
    res = Wire.endTransmission();
  } while (toTx && res == I2C_SUCCESS);
  return res;
}
```

### Register Read Pattern

```c
static int8_t i2cRxReg(dptr, slaveAddr, regAddr, toRx, rxData) {
  // Step 1: Write register address (no data)
  int8_t res = i2cTxOnly(logLevel, slaveAddr, regAddr, 0, 0);
  
  // Step 2: Read data
  if (res == I2C_SUCCESS) {
    res = i2cRxOnly(logLevel, slaveAddr, toRx, rxData);
  }
  return res;
}
```

### Histogram Reading Quirk

Histograms must be read in reverse order to avoid bank switching issues:

```c
int8_t tmf8806ReadQuadHistogram(dptr, slaveAddr, buffer) {
  // Read 128 bytes in 32-byte chunks, starting from end
  for (int8_t i = 128 - 32; i >= 0; i -= 32) {
    i2cRxReg(dptr, slaveAddr, 0x20 + i, 32, buffer + i);
  }
  // Note: Reading register 0x30 causes bank switch!
}
```

### Key I2C Quirks Discovered

1. **Auto-increment:** Registers auto-increment on sequential reads
2. **Bank switching:** Reading certain registers changes the active register bank
3. **Block limits:** Keep transactions ≤31 data bytes for reliable operation
4. **Result read atomicity:** Read 0x1C-0x27 as a block for valid sys_clock

---

## 10. What We Should Copy vs Rewrite

### ✅ COPY (Essential Patterns)

| Pattern | Reason |
|---------|--------|
| Boot sequence timing | Critical for reliable startup |
| Command timeout values | Well-tested, device-specific |
| Data structure layouts | Must match device expectations |
| Serialization functions | Handle bitfield/endianness |
| Register addresses | Hardware-defined |
| Default configuration values | Known-good starting point |
| Reliability interpretation | Device-specific meaning |
| Maximum distance clamping | Per-mode limits |

### ❌ REWRITE (Over-Engineered for Us)

| Component | Replacement |
|-----------|-------------|
| Shim layer | Adafruit_BusIO directly |
| Driver struct with all state | Lean C++ class |
| Clock correction | Optional feature, simpler implementation |
| Histogram dumping | Not needed for basic library |
| Logging infrastructure | Arduino Serial.print() |
| Binary command interface | Not needed |
| Remote control mode | Not needed initially |

### ⚠️ SIMPLIFY

| Feature | Simplified Approach |
|---------|---------------------|
| tmf8806Driver struct | Keep only essential state |
| Configuration handling | Direct register writes, no serialize/deserialize |
| Error handling | Simple bool returns |
| Multiple configs | Single config struct |
| I2C chunking | Adafruit_BusIO handles this |

### 🔑 KEY LEARNINGS

1. **ROM firmware is sufficient** for 2.5m and 5m modes—no download needed
2. **Factory calibration is optional** but improves accuracy significantly  
3. **State data save/restore** is only needed for ultra-low-power sleep modes
4. **Clock correction** adds complexity; skip for v1.0, add later if needed
5. **The shim pattern is unnecessary** with Adafruit_BusIO
6. **Result reading** must be a block read from 0x1C for valid timestamps

---

## Comparison with DESIGN.md

### Things DESIGN.md Got Right

✅ Register addresses and bit definitions  
✅ Boot sequence general flow  
✅ Command protocol (write params, write cmd, poll)  
✅ Interrupt handling (write 1 to clear)  
✅ Little-endian byte order  
✅ Result structure layout  

### Things the Driver Clarifies

| Topic | DESIGN.md Said | Driver Shows |
|-------|----------------|--------------|
| Startup timing | "Wait 1.6ms" | Actually wait 2ms + poll CPU_READY |
| ROM vs RAM app | Unclear | ROM supports 2.5m/5m, RAM only for 10m |
| Calibration loading | "Write to 0x20-0x2D" | Write to 0x20 BEFORE cmd params |
| Stop timeout | "Wait for 0x00" | Timeout = 5ms + (kIters/18) |
| State data | "11 bytes" | Specific format with breakdown voltage |
| Max distance | Per-mode values | 2650/5300/10600mm depending on mode |
| sys_clock validity | "LSB bit is 1" | Confirmed—block read required |

### Things DESIGN.md Missed

1. **Shutdown wait** - After enable LOW, wait 150×10µs for shutdown
2. **Version publish delay** - 10ms wait after RAM remap
3. **Command error check** - Must poll STATE register for errors
4. **Calibration data serialization** - Bitfields need explicit packing
5. **Clock correction formula** - Complex UQ1.15 ratio calculation
6. **SPAD dead time settings** - Values 0-7 map to 97ns-4ns

### Recommendations for Implementation

1. **Start simple:** ROM app, no calibration, no clock correction
2. **Add calibration** as second feature
3. **Consider 10m mode** as advanced optional feature requiring firmware download
4. **Skip histogram reading** entirely for v1.0
5. **Use Adafruit_BusIO** `read()` and `write()` for all I2C
6. **Store config as struct**, serialize only when writing to device

---

## Summary

The AMS driver is well-structured but over-engineered for a simple Arduino library. Key takeaways:

1. **Boot sequence** is straightforward: enable → PON → wait CPU ready → switch app → configure → measure
2. **Command protocol** is simple: write params → write cmd → poll → read result
3. **10m mode requires firmware download**; 2.5m/5m work with ROM
4. **Factory calibration is 14 bytes**, algorithm state is 11 bytes
5. **I2C is standard** with auto-increment and 32-byte chunk limits
6. **Most complexity** (clock correction, histograms, logging) can be omitted initially

Our Adafruit library should be 1/3 the code size with equivalent core functionality.
