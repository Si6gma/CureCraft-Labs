#ifndef I2C_PROTOCOL_H
#define I2C_PROTOCOL_H

#include <cstdint>

/**
 * @brief I2C Protocol Definitions for SensorHub Communication
 * 
 * This protocol defines the communication between Raspberry Pi (I2C master)
 * and SAMD21 SensorHub (I2C slave at address 0x08).
 * 
 * The SAMD21 hub multiplexes sensor access across multiple I2C buses.
 */

// ============================================================================
// I2C Addresses
// ============================================================================

/// SAMD21 SensorHub I2C slave address
constexpr uint8_t HUB_I2C_ADDRESS = 0x08;

// ============================================================================
// Protocol Commands
// ============================================================================

/// Command codes sent from Pi to Hub
enum class HubCommand : uint8_t
{
    PING = 0x01,           ///< Health check - Hub responds with 0xAA
    READ_SENSOR = 0x02,    ///< Read sensor value - Request: [cmd, sensor_id], Response: [4-byte float]
    SCAN_SENSORS = 0x03,   ///< Scan for attached sensors - Response: [status_byte]
    GET_STATUS = 0x04      ///< Get detailed status - Response: [5-byte status array]
};

// ============================================================================
// Sensor Identifiers
// ============================================================================

/// Sensor IDs for READ_SENSOR command
enum class SensorId : uint8_t
{
    ECG = 0x00,           ///< ECG sensor (on W1 bus, typically 0x40)
    SPO2 = 0x01,          ///< SpO2 sensor (on W1 bus, typically 0x41)
    TEMPERATURE = 0x02,   ///< Temperature sensor (on W2 bus, typically 0x42)
    NIBP = 0x03,          ///< NIBP sensor (on W2 bus, typically 0x43)
    RESPIRATORY = 0x04    ///< Respiratory (derived signal, no physical sensor)
};

// ============================================================================
// Response Codes
// ============================================================================

/// Standard response for PING command
constexpr uint8_t PING_RESPONSE = 0xAA;

/// Error code for invalid sensor or failed read
constexpr uint8_t ERROR_RESPONSE = 0xFF;

// ============================================================================
// Status Byte Bit Masks (SCAN_SENSORS response)
// ============================================================================

/// Bit mask for sensor presence in status byte
namespace SensorStatusBits
{
    constexpr uint8_t ECG = (1 << 0);           // Bit 0: ECG attached
    constexpr uint8_t SPO2 = (1 << 1);          // Bit 1: SpO2 attached
    constexpr uint8_t TEMPERATURE = (1 << 2);   // Bit 2: Temperature attached
    constexpr uint8_t NIBP = (1 << 3);          // Bit 3: NIBP attached
    constexpr uint8_t RESPIRATORY = (1 << 4);   // Bit 4: Respiratory available
}

// ============================================================================
// Protocol Constants
// ============================================================================

/// Maximum time to wait for response (milliseconds)
constexpr uint32_t RESPONSE_TIMEOUT_MS = 100;

/// Number of retries for failed commands
constexpr uint8_t MAX_RETRIES = 3;

#endif // I2C_PROTOCOL_H
