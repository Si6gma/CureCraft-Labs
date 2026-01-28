# SensorHub Firmware

Arduino firmware for SAMD21-based sensor hub that acts as an I2C bridge between Raspberry Pi 400 and multiple sensor modules.

## Hardware Setup

### Connections to Raspberry Pi 400

| Pi 400 Pin | Signal | SAMD21 Pin | Signal |
|------------|--------|------------|--------|
| GPIO 8 (Pin 24) | SDA | PA23 (26) | SDA (W0) |
| GPIO 9 (Pin 21) | SCL | PA22 (27) | SCL (W0) |
| GND | Ground | GND | Ground |

**Important:** I2C bus 3 must be enabled on the Pi 400.

### Sensor Connections

The SAMD21 has three I2C buses:

- **W0 (Backbone):** I2C slave to Pi 400 at address `0x08`
- **W1 (Sensors A):** I2C master to ECG and SpO2 sensors
- **W2 (Sensors B):** I2C master to Temperature and NIBP sensors

## Installation

### Pi 400 Setup

1. Enable I2C bus 3:
   ```bash
   sudo nano /boot/config.txt
   ```
   
   Add this line:
   ```
   dtoverlay=i2c3,pins_8_9
   ```

2. Reboot:
   ```bash
   sudo reboot
   ```

3. Verify I2C bus 3 exists:
   ```bash
   ls /dev/i2c-*
   # Should show: /dev/i2c-1  /dev/i2c-3
   ```

4. Scan for the hub (should detect at address 0x08):
   ```bash
   sudo i2cdetect -y 3
   ```

### SAMD21 Firmware Upload

1. Open `sensor_hub_firmware.ino` in Arduino IDE
2. Select your SAMD21 board from Tools → Board
3. Select the correct port from Tools → Port
4. Click Upload
5. Open Serial Monitor at 115200 baud to see debug output

## Protocol

The hub implements a simple command-response protocol:

### Commands

| Command | Code | Request | Response | Description |
|---------|------|---------|----------|-------------|
| PING | 0x01 | `[0x01]` | `[0xAA]` | Health check |
| READ_SENSOR | 0x02 | `[0x02, sensor_id]` | `[4-byte float]` | Read sensor value |
| SCAN_SENSORS | 0x03 | `[0x03]` | `[status_byte]` | Get sensor bitmap |
| GET_STATUS | 0x04 | `[0x04]` | `[5 bytes]` | Detailed status |

### Sensor IDs

| Sensor | ID | Bus | Default Address |
|--------|-----|-----|----------------|
| ECG | 0x00 | W1 | 0x40 |
| SpO2 | 0x01 | W1 | 0x41 |
| Temperature | 0x02 | W2 | 0x42 |
| NIBP | 0x03 | W2 | 0x43 |
| Respiratory | 0x04 | N/A | Derived signal |

## Testing

### Manual I2C Test

Test communication from the Pi:

```bash
# Enable I2C bus 3
sudo modprobe i2c-dev

# Scan for devices
sudo i2cdetect -y 3

# Read one byte (should get PING response 0xAA after sending 0x01)
i2cset -y 3 0x08 0x01
i2cget -y 3 0x08
```

### Serial Monitor Output

When the Pi communicates with the hub, you should see output like:

```
[Hub] Command: 0x01
[Hub] -> PING_RESPONSE
[Hub] Command: 0x03
[Hub] Scanning sensors...
  ECG: YES
  SpO2: YES
  Temp: NO
  NIBP: NO
[Hub] -> Status: 0x03
```

## Troubleshooting

### "SensorHub not responding" on Pi

- Check wiring connections (especially ground)
- Verify I2C bus 3 is enabled and shows /dev/i2c-3
- Run `sudo i2cdetect -y 3` to check if hub appears at 0x08
- Check SAMD21 Serial Monitor for activity

### "Sensors not detected" on Serial Monitor

- Verify sensor modules are properly connected to W1/W2 buses
- Check sensor I2C addresses match expected values (0x40-0x43)
- Add pull-up resistors (4.7k) on W1/W2 buses if needed

### LED Not Blinking

- LED should blink once per command received
- If no blinking, check if Pi is sending commands
- Verify I2C connection between Pi and SAMD21

## Files

- `sensor_hub_firmware.ino` - Main firmware file
- `protocol.h` - Protocol definitions (shared with Pi code)
- `README.md` - This file
