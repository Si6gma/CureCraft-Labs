#include "i2c_driver.h"
#include <iostream>
#include <cstring>
#include <cmath>

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

bool I2CDriver::deviceExists(uint8_t address)
{
    if (mockMode_)
    {
        // In mock mode, simulate some devices present
        return (address >= 0x40 && address <= 0x44);
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

bool I2CDriver::readFloat(uint8_t address, float& value)
{
    if (mockMode_)
    {
        value = generateMockValue(address);
        return true;
    }

    uint8_t buffer[4];
    if (!readBytes(address, buffer, 4))
    {
        return false;
    }

    // Convert little-endian bytes to float
    std::memcpy(&value, buffer, sizeof(float));
    return true;
}

float I2CDriver::generateMockValue(uint8_t address)
{
    // Generate realistic mock values based on sensor type (address)
    static double time = 0.0;
    time += 0.05; // Increment time

    switch (address)
    {
        case 0x40: // ECG
            return 0.8f + 0.2f * std::sin(2.0 * M_PI * 1.0 * time);
        case 0x41: // SpO2
            return 97.0f + 2.0f * std::sin(2.0 * M_PI * 1.2 * time);
        case 0x42: // Temperature 1 (cavity)
            return 37.2f + 0.1f * std::sin(2.0 * M_PI * 0.01 * time);
        case 0x43: // Temperature 2 (skin)
            return 36.8f + 0.1f * std::sin(2.0 * M_PI * 0.01 * time);
        case 0x44: // NIBP
            return 120.0f; // Systolic
        default:
            return 0.0f;
    }
}
