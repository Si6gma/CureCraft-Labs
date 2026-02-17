#include "hardware/i2c_driver.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <thread>
#include <chrono>

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#endif

namespace {
    constexpr int PING_DELAY_MS = 5;
    constexpr int SENSOR_READ_DELAY_MS = 10;
    constexpr int SCAN_DELAY_MS = 50;
    constexpr int STATUS_DELAY_MS = 10;
    constexpr int BUS_READY_DELAY_MS = 2;
    constexpr double MOCK_TIME_INCREMENT = 0.05;
}

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
        std::cout << "[I2C Mock] PING -> 0x42" << std::endl;
        return true;
    }

#ifdef __linux__
    if (fd_ < 0)
        return false;

    uint8_t cmd = static_cast<uint8_t>(HubCommand::PING);
    if (!writeByte(HUB_I2C_ADDRESS, cmd))
    {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(PING_DELAY_MS));

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

bool I2CDriver::readSensor(SensorId sensorId, float &value)
{
    if (mockMode_)
    {
        value = generateMockValue(sensorId);
        return true;
    }

#ifdef __linux__
    if (fd_ < 0)
        return false;

    uint8_t cmd = static_cast<uint8_t>(HubCommand::READ_SENSOR);
    uint8_t id = static_cast<uint8_t>(sensorId);

    if (!writeCommand(HUB_I2C_ADDRESS, cmd, id))
    {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(SENSOR_READ_DELAY_MS));

    uint8_t buffer[4];
    if (!readBytes(HUB_I2C_ADDRESS, buffer, 4))
    {
        return false;
    }

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
        // In mock mode, return a status indicating all sensors available
        return 0x1F; // All 5 bits set: ECG|SpO2|CoreTemp|NIBP|SkinTemp
    }

#ifdef __linux__
    if (fd_ < 0)
        return 0xFF;

    uint8_t cmd = static_cast<uint8_t>(HubCommand::SCAN_SENSORS);
    if (!writeByte(HUB_I2C_ADDRESS, cmd))
    {
        std::cerr << "[I2C] Failed to send scan command" << std::endl;
        return 0xFF;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(SCAN_DELAY_MS));

    uint8_t status = 0;
    if (!readByte(HUB_I2C_ADDRESS, status))
    {
        std::cerr << "[I2C] Failed to read scan results" << std::endl;
        return 0xFF;
    }

    return status;
#else
    return 0xFF;
#endif
}
bool I2CDriver::getSensorStatus(uint8_t *statusBuffer)
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
    if (fd_ < 0)
        return false;

    uint8_t cmd = static_cast<uint8_t>(HubCommand::GET_STATUS);
    if (!writeByte(HUB_I2C_ADDRESS, cmd))
    {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(STATUS_DELAY_MS));

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
    if (!isOpen())
    {
        return false;
    }

    if (mockMode_)
    {
        // In mock mode, only the hub exists
        bool exists = (address == HUB_I2C_ADDRESS);
        std::cout << "[I2C] Mock: Device 0x" << std::hex << (int)address << std::dec
                  << (exists ? " EXISTS" : " not found") << std::endl;
        return exists;
    }

#ifdef __linux__
    std::cout << "[I2C] Probing device at 0x" << std::hex << (int)address << std::dec << "..." << std::endl;

    if (ioctl(fd_, I2C_SLAVE, address) < 0)
    {
        std::cerr << "[I2C] Failed to select device 0x" << std::hex << (int)address << std::dec << std::endl;
        return false;
    }

    uint8_t byte;
    if (read(fd_, &byte, 1) != 1)
    {
        std::cout << "[I2C] Device 0x" << std::hex << (int)address << std::dec << " not responding" << std::endl;
        return false;
    }

    std::cout << "[I2C] ✓ Device 0x" << std::hex << (int)address << std::dec << " detected" << std::endl;
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
    if (fd_ < 0)
    {
        std::cerr << "[I2C] File descriptor invalid" << std::endl;
        return false;
    }

    if (ioctl(fd_, I2C_SLAVE, address) < 0)
    {
        std::cerr << "[I2C] Failed to set slave address 0x" << std::hex << (int)address
                  << std::dec << ": " << strerror(errno) << std::endl;
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(BUS_READY_DELAY_MS));

    if (write(fd_, &data, 1) != 1)
    {
        std::cerr << "[I2C] Failed to write byte to 0x" << std::hex << (int)address
                  << std::dec << ": " << strerror(errno) << std::endl;
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
    if (fd_ < 0)
    {
        std::cerr << "[I2C] File descriptor invalid" << std::endl;
        return false;
    }

    if (ioctl(fd_, I2C_SLAVE, address) < 0)
    {
        std::cerr << "[I2C] Failed to set slave address 0x" << std::hex << (int)address
                  << std::dec << ": " << strerror(errno) << std::endl;
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(BUS_READY_DELAY_MS));

    uint8_t buffer[2] = {command, data};
    if (write(fd_, buffer, 2) != 2)
    {
        std::cerr << "[I2C] Failed to write command to 0x" << std::hex << (int)address
                  << std::dec << ": " << strerror(errno) << std::endl;
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool I2CDriver::readByte(uint8_t address, uint8_t &data)
{
    if (mockMode_)
    {
        data = 0xAA; // Mock response
        return true;
    }

#ifdef __linux__
    if (fd_ < 0)
    {
        std::cerr << "[I2C] File descriptor invalid" << std::endl;
        return false;
    }

    if (ioctl(fd_, I2C_SLAVE, address) < 0)
    {
        std::cerr << "[I2C] Failed to set slave address 0x" << std::hex << (int)address
                  << std::dec << ": " << strerror(errno) << std::endl;
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(BUS_READY_DELAY_MS));

    if (read(fd_, &data, 1) != 1)
    {
        std::cerr << "[I2C] Failed to read byte from 0x" << std::hex << (int)address
                  << std::dec << ": " << strerror(errno) << std::endl;
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool I2CDriver::readBytes(uint8_t address, uint8_t *buffer, size_t length)
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
    if (fd_ < 0)
    {
        std::cerr << "[I2C] File descriptor invalid" << std::endl;
        return false;
    }

    if (ioctl(fd_, I2C_SLAVE, address) < 0)
    {
        std::cerr << "[I2C] Failed to set slave address 0x" << std::hex << (int)address
                  << std::dec << ": " << strerror(errno) << std::endl;
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(BUS_READY_DELAY_MS));

    if (read(fd_, buffer, length) != (ssize_t)length)
    {
        std::cerr << "[I2C] Failed to read " << length << " bytes from 0x" << std::hex << (int)address
                  << std::dec << ": " << strerror(errno) << std::endl;
        return false;
    }

    return true;
#else
    return false;
#endif
}

// ============================================================================
// Mock Data Generation - Realistic Medical Waveforms
// ============================================================================

// Helper function: Generate realistic ECG waveform (Normal Sinus Rhythm)
float generateECGWaveform(double time) {
    const double HR = 75.0; // Heart rate in BPM
    const double beatInterval = 60.0 / HR; // ~0.8 seconds per beat
    
    // Position within current beat cycle (0.0 to 1.0)
    double beatPhase = std::fmod(time, beatInterval) / beatInterval;
    
    // Baseline
    float value = 0.0f;
    
    // P wave (atrial depolarization) at 0.0-0.1 of cycle
    if (beatPhase < 0.1) {
        double pPhase = beatPhase / 0.1;
        value = 0.15f * std::exp(-50.0 * std::pow(pPhase - 0.5, 2));
    }
    // PR segment (isoelectric) at 0.1-0.2
    else if (beatPhase < 0.2) {
        value = 0.0f;
    }
    // QRS complex (ventricular depolarization) at 0.2-0.3
    else if (beatPhase < 0.3) {
        double qrsPhase = (beatPhase - 0.2) / 0.1;
        // Q wave (small negative deflection)
        if (qrsPhase < 0.2) {
            value = -0.1f * (qrsPhase / 0.2);
        }
        // R wave (large positive spike)
        else if (qrsPhase < 0.6) {
            double rPhase = (qrsPhase - 0.2) / 0.4;
            value = 1.0f * std::exp(-25.0 * std::pow(rPhase - 0.5, 2));
        }
        // S wave (negative deflection)
        else {
            double sPhase = (qrsPhase - 0.6) / 0.4;
            value = -0.2f * std::exp(-25.0 * std::pow(sPhase - 0.3, 2));
        }
    }
    // ST segment at 0.3-0.4
    else if (beatPhase < 0.4) {
        value = 0.0f;
    }
    // T wave (ventricular repolarization) at 0.4-0.7
    else if (beatPhase < 0.7) {
        double tPhase = (beatPhase - 0.4) / 0.3;
        value = 0.3f * std::exp(-8.0 * std::pow(tPhase - 0.5, 2));
    }
    // Return to baseline
    else {
        value = 0.0f;
    }
    
    // Scale to typical ECG voltage range and add baseline offset
    return 0.5f + value * 0.4f;
}

// Helper function: Generate pulsatile waveform for SpO2/Pleth
float generatePlethWaveform(double time) {
    const double HR = 75.0; // Match ECG heart rate
    const double beatInterval = 60.0 / HR;
    
    double beatPhase = std::fmod(time, beatInterval) / beatInterval;
    
    float value = 0.0f;
    
    if (beatPhase < 0.3) {
        // Rapid systolic upstroke
        double upstrokePhase = beatPhase / 0.3;
        value = std::pow(upstrokePhase, 2.0);
    }
    else if (beatPhase < 0.5) {
        // Dicrotic notch
        double notchPhase = (beatPhase - 0.3) / 0.2;
        value = 1.0f - 0.15f * std::sin(notchPhase * M_PI);
    }
    else {
        // Diastolic decay
        double decayPhase = (beatPhase - 0.5) / 0.5;
        value = (1.0f - 0.15f) * std::exp(-3.0 * decayPhase);
    }
    
    // Add small baseline noise for realism
    value += 0.02f * std::sin(2.0 * M_PI * 15.0 * time);
    
    return value;
}

// Helper function: Generate realistic respiratory waveform
float generateRespiratoryWaveform(double time) {
    const double RR = 14.0; // Respiratory rate in breaths/min
    const double breathInterval = 60.0 / RR; // ~4.3 seconds per breath
    
    double breathPhase = std::fmod(time, breathInterval) / breathInterval;
    
    float value;
    
    // Inhalation is faster (40% of cycle), exhalation is slower (60%)
    if (breathPhase < 0.4) {
        // Inhalation - steeper curve
        double inhalePhase = breathPhase / 0.4;
        value = 0.5f * (1.0f - std::cos(inhalePhase * M_PI));
    }
    else {
        // Exhalation - gentler curve
        double exhalePhase = (breathPhase - 0.4) / 0.6;
        value = 0.5f * (1.0f + std::cos(exhalePhase * M_PI));
    }
    
    // Add slight amplitude variation for natural breathing
    value *= (1.0f + 0.1f * std::sin(2.0 * M_PI * 0.05 * time));
    
    return value;
}

float I2CDriver::generateMockValue(SensorId sensorId)
{
    static double time = 0.0;
    time += MOCK_TIME_INCREMENT;

    switch (sensorId)
    {
    case SensorId::ECG:
        return generateECGWaveform(time);
        
    case SensorId::SPO2:
        // SpO2 percentage - should be relatively stable, not a waveform
        // Realistic range: 96-99% with slow variation
        return 97.5f + 1.0f * std::sin(2.0 * M_PI * 0.02 * time);
        
    case SensorId::TEMP_CORE:
        // Core temperature with slow drift
        return 37.2f + 0.05f * std::sin(2.0 * M_PI * 0.01 * time);
        
    case SensorId::TEMP_SKIN:
        // Skin temperature with slightly more variation
        return 36.5f + 0.1f * std::sin(2.0 * M_PI * 0.01 * time);
        
    case SensorId::NIBP:
        // Blood pressure (static for now, would need NIBP-specific updates)
        return 120.0f;
        
    case SensorId::RESPIRATORY:
        return generateRespiratoryWaveform(time);
        
    default:
        return 0.0f;
    }
}


uint8_t I2CDriver::generateMockStatusByte()
{
    // In mock mode, all sensors are present
    return SensorStatusBits::ECG |
           SensorStatusBits::SPO2 |
           SensorStatusBits::TEMP_CORE |
           SensorStatusBits::NIBP |
           SensorStatusBits::TEMP_SKIN |
           SensorStatusBits::RESPIRATORY;
}
