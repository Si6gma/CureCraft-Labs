# SAMD21 SensorHub Firmware

## Overview

This firmware runs on the SAMD21 SensorHub board and enables automatic sensor detection for the CureCraft Patient Monitor.

**Key Features:**
- 🔄 **Automatic Sensor Polling**: Scans all sensor buses every 5 seconds
- 🔌 **Hot-Plug Detection**: Automatically detects when sensors are connected/disconnected
- 📡 **I2C Slave Interface**: Communicates with Raspberry Pi at address 0x08
- 🔍 **Multi-Sensor Support**: ECG, SpO2, Temperature, and NIBP sensors

## Hardware Connections

### Raspberry Pi 400 Connection
- **GPIO 2 (SDA)** → SAMD21 PA23 (W0_SDA)
- **GPIO 3 (SCL)** → SAMD21 PA22 (W0_SCL)
- This is **I2C Bus 1** on the Pi

### Sensor Buses
- **Bus A (W1)**: PA12 (SDA) / PA13 (SCL) → ECG (0x40), SpO2 (0x41), or Temp (0x68)
- **Bus B (W2)**: PA16 (SDA) / PA17 (SCL) → NIBP (0x43) or Temp (0x68)

## I2C Protocol

### Hub Address
- `0x08` - SAMD21 SensorHub address on Pi's I2C bus

### Automatic Sensor Scanning

⚡ **The hub automatically scans sensor buses every 5 seconds** and maintains a cached status byte. This enables hot-plug detection without Pi intervention.

When a sensor is connected or disconnected, the hub logs the event to Serial and updates the cached status within 5 seconds.

### Commands

#### PING (0x00)
Check if hub is alive.

```
Pi sends:    0x00
Pi reads:    0x42 (magic response)
```

#### SCAN (0x01)
Get cached sensor status (does NOT trigger a new scan).

```
Pi sends:    0x01
Pi reads:    Status byte (most recent scan result)
```

**Status Byte Format:**
- Bit 0: ECG present (0x40 on W1)
- Bit 1: SpO2 present (0x41 on W1)
- Bit 2: Temperature present (0x68 on W1 or W2)
- Bit 3: NIBP present (0x43 on W2)
- Bits 4-7: Reserved (0)

**Example:**
- `0b00000101` (0x05) = ECG and Temperature detected
- `0b00001111` (0x0F) = All 4 sensors detected
- `0b00000100` (0x04) = Only Temperature detected

## Uploading Firmware

### Using Arduino IDE

1. Install Arduino SAMD boards support
2. Install required libraries: `Wire`, `TwiPinHelper`
3. Select **Board**: "Arduino Zero" or your SAMD21 variant
4. Select **Port**: /dev/ttyACM0 (or similar)
5. Upload `sensor_hub_scanner.ino`

### Verify Upload

Open Serial Monitor (115200 baud):

```
========================================
  SensorHub Scanner Firmware v1.0
========================================
Hub Address: 0x08
Backbone: PA22/PA23 (to Pi)
Sensor A: PA12/PA13 (W1)
Sensor B: PA16/PA17 (W2)
Ready!

========================================
--- Auto-scanning Sensors ---
Time: 0s
========================================
Bus A (W1):
  ✗ ECG (0x40)
  ✗ SpO2 (0x41)
  ✓ Temperature (0x68)
Bus B (W2):
  ✗ NIBP (0x43)
========================================
Status byte: 0b100 (0x4)
========================================

[... automatic scans every 5 seconds ...]

>>> SENSOR STATUS CHANGED <<<
Previous: 0b100 -> New: 0b101
```

## Testing from Raspberry Pi

```bash
# Check hub responds
sudo i2cdetect -y 1
# Should show device at 0x08

# Send PING command
sudo i2cset -y 1 0x08 0x00
sudo i2cget -y 1 0x08
# Should return: 0x42

# Send SCAN command  
sudo i2cset -y 1 0x08 0x01
sudo i2cget -y 1 0x08
# Returns status byte (e.g., 0x04 for temp sensor only)
```

## LED Indicator

- **Blinks slowly**: Normal operation, waiting for commands
- **Solid**: System starting up

## Troubleshooting

**Hub not detected by Pi:**
- Check I2C wiring (GPIO 2/3)
- Verify power to SAMD21
- Check Serial Monitor for startup message

**Sensors not detected:**
- Verify sensor I2C addresses  
- Check W1/W2 bus wiring
- Test sensors with `i2cdetect` if directly connected to Pi

**No Serial output:**
- Check baud rate (115200)
- Ensure USB/Serial connection active
- Some boards may need DTR/RTS toggle


## Common Issues & Solutions

### Bus B Scanning Hangs

**Symptom**: Firmware hangs at "Bus B (W2):" with no output after.

**Root Cause**: Incorrect SERCOM configuration and pin peripheral states cause I2C bus lockup.

**Solution**: Ensure correct SERCOM assignments and pin states:
```cpp
// Correct SERCOM assignments
TwoWire WireBackbone(&sercom3, W0_SDA, W0_SCL);  // SERCOM3 for backbone
TwoWire WireSensorA(&sercom1, W1_SDA, W1_SCL);   // SERCOM1 for W1
TwoWire WireSensorB(&sercom4, W2_SDA, W2_SCL);   // SERCOM4 for W2

// In setup():
portBackbone.setPinPeripheralStates();      // NOT AltStates
portSensorsA.setPinPeripheralAltStates();   // AltStates for SERCOM1
portSensorsB.setPinPeripheralStates();      // NOT AltStates

// Interrupt handler
void SERCOM3_Handler(void) {  // SERCOM3, not SERCOM5
    WireBackbone.onService();
}
```

## Files

- `sensor_hub.ino` - Main firmware (use this!)
- `TwiPinHelper.h` - Helper library for SERCOM pin configuration
- `README.md` - This file
