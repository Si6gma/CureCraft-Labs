#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <string>
#include <cstdint>

/**
 * @brief I²C driver for communicating with SAMD21-based sensor modules
 * 
 * This driver provides low-level I²C communication for reading sensor data
 * from SAMD21 microcontroller modules. It supports both real hardware
 * (Linux I²C via /dev/i2c-*) and mock mode for development/testing.
 */
class I2CDriver
{
public:
    /**
     * @brief Construct I²C driver
     * @param bus I²C bus number (default: 1 for /dev/i2c-1)
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

    /**
     * @brief Read float value from device (4 bytes, little-endian)
     * @param address 7-bit I²C address
     * @param value Output float value
     * @return true if successful
     */
    bool readFloat(uint8_t address, float& value);

private:
    int bus_;           // I²C bus number
    int fd_;            // File descriptor for I²C device
    bool mockMode_;     // Mock mode flag

    // Mock data generation
    float generateMockValue(uint8_t address);
};

#endif // I2C_DRIVER_H
