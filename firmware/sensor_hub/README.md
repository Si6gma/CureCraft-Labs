# SAMD21 SensorHub Firmware

## Overview

This firmware runs on the SAMD21 SensorHub board and enables sensor detection for the CureCraft Patient Monitor.

## Hardware Connections

### Raspberry Pi 400 Connection
- **GPIO 2 (SDA)** → SAMD21 PA23 (W0_SDA)
- **GPIO 3 (SCL)** → SAMD21 PA22 (W0_SCL)
- This is **I2C Bus 1** on the Pi

### Sensor Buses
- **Bus A (W1)**: PA12 (SDA) / PA13 (SCL) → ECG (0x40), SpO2 (0x41)
- **Bus B (W2)**: PA16 (SDA) / PA17 (SCL) → Temp (0x42), NIBP (0x43)

## I2C Protocol

### Hub Address
- `0x08` - SAMD21 SensorHub address on Pi's I2C bus

### Commands

#### PING (0x00)
Check if hub is alive.

```
Pi sends:    0x00
Pi reads:    0x42 (magic response)
```

#### SCAN (0x01)
Scan for connected sensors.

```
Pi sends:    0x01
Pi reads:    Status byte (bit field)
```

**Status Byte Format:**
- Bit 0: ECG present (0x40)
- Bit 1: SpO2 present (0x41)
- Bit 2: Temperature present (0x42)
- Bit 3: NIBP present (0x43)
- Bits 4-7: Reserved (0)

**Example:**
- `0b00000101` (0x05) = ECG and Temp detected
- `0b00001111` (0x0F) = All sensors detected

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

--- Scanning Sensors ---
Bus A (W1):
  ✗ ECG
  ✗ SpO2
Bus B (W2):
  ✓ Temp (0x42)
  ✗ NIBP
Status byte: 0b00000100
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

## Files

- `sensor_hub_scanner.ino` - Main firmware (use this!)
- `TwiPinHelper.h` - Helper library for SERCOM pin configuration
- `README.md` - This file
