/**
 * SensorHub I2C Protocol Definitions
 * 
 * Shared protocol between Raspberry Pi (master) and SAMD21 SensorHub (slave)
 * 
 * This file should be kept in sync with the C++ version:
 * include/hardware/i2c_protocol.h
 */

#ifndef I2C_PROTOCOL_H
#define I2C_PROTOCOL_H

#include <Arduino.h>

// ============================================================================
// I2C Addresses
// ============================================================================

/// SAMD21 SensorHub I2C slave address
#define HUB_I2C_ADDRESS 0x08

// ============================================================================
// Protocol Commands
// ============================================================================

/// Command codes sent from Pi to Hub
enum HubCommand : uint8_t
{
    CMD_PING = 0x01,           ///< Health check - Hub responds with 0xAA
    CMD_READ_SENSOR = 0x02,    ///< Read sensor value - Request: [cmd, sensor_id], Response: [4-byte float]
    CMD_SCAN_SENSORS = 0x03,   ///< Scan for attached sensors - Response: [status_byte]
    CMD_GET_STATUS = 0x04      ///< Get detailed status - Response: [5-byte status array]
};

// ============================================================================
// Sensor Identifiers
// ============================================================================

/// Sensor IDs for READ_SENSOR command
enum SensorId : uint8_t
{
    SENSOR_ECG = 0x00,           ///< ECG sensor (on W1 bus, typically 0x40)
    SENSOR_SPO2 = 0x01,          ///< SpO2 sensor (on W1 bus, typically 0x41)
    SENSOR_TEMPERATURE = 0x02,   ///< Temperature sensor (on W2 bus, typically 0x42)
    SENSOR_NIBP = 0x03,          ///< NIBP sensor (on W2 bus, typically 0x43)
    SENSOR_RESPIRATORY = 0x04    ///< Respiratory (derived signal, no physical sensor)
};

// ============================================================================
// Sensor I2C Addresses (on W1/W2 buses)
// ============================================================================

#define ADDR_ECG        0x40
#define ADDR_SPO2       0x41
#define ADDR_TEMP       0x42
#define ADDR_NIBP       0x43

// ============================================================================
// Response Codes
// ============================================================================

/// Standard response for PING command
#define PING_RESPONSE 0xAA

/// Error code for invalid sensor or failed read
#define ERROR_RESPONSE 0xFF

// ============================================================================
// Status Byte Bit Masks (SCAN_SENSORS response)
// ============================================================================

/// Bit mask for sensor presence in status byte
#define STATUS_BIT_ECG          (1 << 0)  // Bit 0: ECG attached
#define STATUS_BIT_SPO2         (1 << 1)  // Bit 1: SpO2 attached
#define STATUS_BIT_TEMPERATURE  (1 << 2)  // Bit 2: Temperature attached
#define STATUS_BIT_NIBP         (1 << 3)  // Bit 3: NIBP attached
#define STATUS_BIT_RESPIRATORY  (1 << 4)  // Bit 4: Respiratory available

#endif // I2C_PROTOCOL_H
