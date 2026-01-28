#include "hardware/i2c_driver.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <thread>
#include <chrono>

// Linux I²C headers (only included on Linux)
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#endif

I2CDriver::I2CDriver(int bus, bool mockMode)
    : bus_(bus), fd_(-1), mockMode_(mockMode)
{
}

I2CDriver::~I2CDriver()
{
    close();
}

bool I2CDriver::open()
{
    if (mockMode_)
    {
        std::cout << "[I2C Mock] Mock mode enabled - no hardware access" << std::endl;
        return true;
    }

#ifdef __linux__
    std::string device = "/dev/i2c-" + std::to_string(bus_);
    fd_ = ::open(device.c_str(), O_RDWR);
    
    if (fd_ < 0)
    {
        std::cerr << "[I2C] Failed to open " << device << ": " << strerror(errno) << std::endl;
        std::cerr << "[I2C] Hint: Run 'sudo raspi-config' to enable I²C" << std::endl;
        return false;
    }
    
    std::cout << "[I2C] Opened " << device << " successfully" << std::endl;
    return true;
#else
    std::cerr << "[I2C] I²C only supported on Linux. Use mock mode on other platforms." << std::endl;
    return false;
#endif
}

void I2CDriver::close()
{
    if (fd_ >= 0)
    {
#ifdef __linux__
        ::close(fd_);
#endif
        fd_ = -1;
    }
}

// ============================================================================
// Hub Protocol Commands
// ============================================================================

bool I2CDriver::pingHub()
{
    if (mockMode_)
    {
        std::cout << "[I2C Mock] PING -> 0xAA" << std::endl;
        return true;
    }

#ifdef __linux__
    if (fd_ < 0) return false;

    // Send PING command
    uint8_t cmd = static_cast<uint8_t>(HubCommand::PING);
    if (!writeByte(HUB_I2C_ADDRESS, cmd))
    {
        return false;
    }

    // Small delay for hub to process
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // Read response
    uint8_t response;
    if (!readByte(HUB_I2C_ADDRESS, response))
    {
        return false;
    }

    return (response == PING_RESPONSE);
#else
    return false;
#endif
}

bool I2CDriver::readSensor(SensorId sensorId, float& value)
{
    if (mockMode_)
    {
        value = generateMockValue(sensorId);
        return true;
    }

#ifdef __linux__
    if (fd_ < 0) return false;

    // Send READ_SENSOR command with sensor ID
    uint8_t cmd = static_cast<uint8_t>(HubCommand::READ_SENSOR);
    uint8_t id = static_cast<uint8_t>(sensorId);
    
    if (!writeCommand(HUB_I2C_ADDRESS, cmd, id))
    {
        return false;
    }

    // Small delay for hub to read from actual sensor
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Read 4-byte float response
    uint8_t buffer[4];
    if (!readBytes(HUB_I2C_ADDRESS, buffer, 4))
    {
        return false;
    }

    // Convert little-endian bytes to float
    std::memcpy(&value, buffer, sizeof(float));
    return true;
#else
    return false;
#endif
}

uint8_t I2CDriver::scanSensors()
{
    if (mockMode_)
    {
        uint8_t status = generateMockStatusByte();
        std::cout << "[I2C Mock] SCAN_SENSORS -> 0x" << std::hex << (int)status << std::dec << std::endl;
        return status;
    }

#ifdef __linux__
    if (fd_ < 0) return 0xFF;

    // Send SCAN_SENSORS command
    uint8_t cmd = static_cast<uint8_t>(HubCommand::SCAN_SENSORS);
    if (!writeByte(HUB_I2C_ADDRESS, cmd))
    {
        return 0xFF;
    }

    // Delay for hub to scan all sensors
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Read status byte
    uint8_t status;
    if (!readByte(HUB_I2C_ADDRESS, status))
    {
        return 0xFF;
    }

    return status;
#else
    return 0xFF;
#endif
}

bool I2CDriver::getSensorStatus(uint8_t* statusBuffer)
{
    if (mockMode_)
    {
        // Generate mock status
        statusBuffer[0] = 1; // ECG
        statusBuffer[1] = 1; // SpO2
        statusBuffer[2] = 1; // Temperature
        statusBuffer[3] = 1; // NIBP
        statusBuffer[4] = 1; // Respiratory
        return true;
    }

#ifdef __linux__
    if (fd_ < 0) return false;

    // Send GET_STATUS command
    uint8_t cmd = static_cast<uint8_t>(HubCommand::GET_STATUS);
    if (!writeByte(HUB_I2C_ADDRESS, cmd))
    {
        return false;
    }

    // Small delay
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Read 5-byte status array
    if (!readBytes(HUB_I2C_ADDRESS, statusBuffer, 5))
    {
        return false;
    }

    return true;
#else
    return false;
#endif
}

// ============================================================================
// Low-Level I²C Operations
// ============================================================================

bool I2CDriver::deviceExists(uint8_t address)
{
    if (mockMode_)
    {
        // In mock mode, simulate hub present
        return (address == HUB_I2C_ADDRESS);
    }

#ifdef __linux__
    if (fd_ < 0) return false;

    // Try to set I²C slave address
    if (ioctl(fd_, I2C_SLAVE, address) < 0)
    {
        return false;
    }

    // Try to read one byte
    uint8_t data;
    if (read(fd_, &data, 1) != 1)
    {
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool I2CDriver::writeByte(uint8_t address, uint8_t data)
{
    if (mockMode_)
    {
        std::cout << "[I2C Mock] Write 0x" << std::hex << (int)data 
                  << " to 0x" << (int)address << std::dec << std::endl;
        return true;
    }

#ifdef __linux__
    if (fd_ < 0) return false;

    if (ioctl(fd_, I2C_SLAVE, address) < 0)
    {
        std::cerr << "[I2C] Failed to set slave address 0x" << std::hex << (int)address << std::dec << std::endl;
        return false;
    }

    if (write(fd_, &data, 1) != 1)
    {
        std::cerr << "[I2C] Failed to write byte" << std::endl;
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool I2CDriver::writeCommand(uint8_t address, uint8_t command, uint8_t data)
{
    if (mockMode_)
    {
        std::cout << "[I2C Mock] Write command 0x" << std::hex << (int)command 
                  << " with data 0x" << (int)data << " to 0x" << (int)address << std::dec << std::endl;
        return true;
    }

#ifdef __linux__
    if (fd_ < 0) return false;

    if (ioctl(fd_, I2C_SLAVE, address) < 0)
    {
        std::cerr << "[I2C] Failed to set slave address 0x" << std::hex << (int)address << std::dec << std::endl;
        return false;
    }

    uint8_t buffer[2] = {command, data};
    if (write(fd_, buffer, 2) != 2)
    {
        std::cerr << "[I2C] Failed to write command" << std::endl;
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool I2CDriver::readByte(uint8_t address, uint8_t& data)
{
    if (mockMode_)
    {
        data = 0xAA; // Mock response
        return true;
    }

#ifdef __linux__
    if (fd_ < 0) return false;

    if (ioctl(fd_, I2C_SLAVE, address) < 0)
    {
        return false;
    }

    if (read(fd_, &data, 1) != 1)
    {
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool I2CDriver::readBytes(uint8_t address, uint8_t* buffer, size_t length)
{
    if (mockMode_)
    {
        // Generate mock data
        for (size_t i = 0; i < length; i++)
        {
            buffer[i] = i;
        }
        return true;
    }

#ifdef __linux__
    if (fd_ < 0) return false;

    if (ioctl(fd_, I2C_SLAVE, address) < 0)
    {
        return false;
    }

    if (read(fd_, buffer, length) != (ssize_t)length)
    {
        return false;
    }

    return true;
#else
    return false;
#endif
}

// ============================================================================
// Mock Data Generation
// ============================================================================

float I2CDriver::generateMockValue(SensorId sensorId)
{
    // Generate realistic mock values based on sensor type
    static double time = 0.0;
    time += 0.05; // Increment time

    switch (sensorId)
    {
        case SensorId::ECG:
            return 0.8f + 0.2f * std::sin(2.0 * M_PI * 1.0 * time);
        case SensorId::SPO2:
            return 97.0f + 2.0f * std::sin(2.0 * M_PI * 1.2 * time);
        case SensorId::TEMPERATURE:
            return 37.2f + 0.1f * std::sin(2.0 * M_PI * 0.01 * time);
        case SensorId::NIBP:
            return 120.0f; // Systolic
        case SensorId::RESPIRATORY:
            return 16.0f + 2.0f * std::sin(2.0 * M_PI * 0.25 * time);
        default:
            return 0.0f;
    }
}

uint8_t I2CDriver::generateMockStatusByte()
{
    // In mock mode, all sensors are present
    return SensorStatusBits::ECG | 
           SensorStatusBits::SPO2 | 
           SensorStatusBits::TEMPERATURE | 
           SensorStatusBits::NIBP | 
           SensorStatusBits::RESPIRATORY;
}
