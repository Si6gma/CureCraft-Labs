#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <string>
#include <cstdint>
#include "hardware/i2c_protocol.h"

/**
 * @brief I²C driver for communicating with SAMD21 SensorHub
 * 
 * This driver provides I²C communication with the SAMD21 SensorHub device.
 * The hub acts as an I²C slave (at address 0x08) and multiplexes access
 * to multiple sensor modules on its own I²C buses.
 * 
 * Supports both real hardware (Linux I²C via /dev/i2c-*) and mock mode
 * for development/testing without physical hardware.
 */
class I2CDriver
{
public:
    /**
     * @brief Construct I²C driver
     * @param bus I²C bus number (default: 1 for GPIO 2/3)
     * @param mockMode Enable mock mode for testing without hardware
     */
    explicit I2CDriver(int bus = 1, bool mockMode = false);
    
    ~I2CDriver();

    /**
     * @brief Open I²C bus connection
     * @return true if successful
     */
    bool open();

    /**
     * @brief Close I²C bus connection
     */
    void close();

    /**
     * @brief Check if I²C bus is open
     * @return true if open
     */
    bool isOpen() const { return fd_ >= 0 || mockMode_; }

    // ========================================================================
    // Hub Protocol Commands
    // ========================================================================

    /**
     * @brief Send PING command to hub
     * @return true if hub responds with correct acknowledgment
     */
    bool pingHub();

    /**
     * @brief Read sensor value from hub
     * @param sensorId Sensor identifier
     * @param value Output float value
     * @return true if successful
     */
    bool readSensor(SensorId sensorId, float& value);

    /**
     * @brief Scan for attached sensors
     * @return Status byte with sensor presence bits, or 0xFF on error
     */
    uint8_t scanSensors();

    /**
     * @brief Get detailed sensor status
     * @param statusBuffer Output buffer (must be at least 5 bytes)
     * @return true if successful
     */
    bool getSensorStatus(uint8_t* statusBuffer);

    // ========================================================================
    // Low-Level I²C Operations (for internal use)
    // ========================================================================

    /**
     * @brief Check if device exists at address
     * @param address 7-bit I²C address
     * @return true if device responds
     */
    bool deviceExists(uint8_t address);

    /**
     * @brief Write single byte to device
     * @param address 7-bit I²C address
     * @param data Byte to write
     * @return true if successful
     */
    bool writeByte(uint8_t address, uint8_t data);

    /**
     * @brief Write command with data
     * @param address 7-bit I²C address
     * @param command Command byte
     * @param data Data byte
     * @return true if successful
     */
    bool writeCommand(uint8_t address, uint8_t command, uint8_t data);

    /**
     * @brief Read single byte from device
     * @param address 7-bit I²C address
     * @param data Output buffer for read byte
     * @return true if successful
     */
    bool readByte(uint8_t address, uint8_t& data);

    /**
     * @brief Read multiple bytes from device
     * @param address 7-bit I²C address
     * @param buffer Output buffer
     * @param length Number of bytes to read
     * @return true if successful
     */
    bool readBytes(uint8_t address, uint8_t* buffer, size_t length);

private:
    int bus_;           // I²C bus number
    int fd_;            // File descriptor for I²C device
    bool mockMode_;     // Mock mode flag

    // Mock data generation
    float generateMockValue(SensorId sensorId);
    uint8_t generateMockStatusByte();
};

#endif // I2C_DRIVER_H
