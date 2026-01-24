#include "hardware/sensor_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>

SensorManager::SensorManager(bool mockMode)
    : mockMode_(mockMode)
{
    i2c_ = std::make_unique<I2CDriver>(1, mockMode);
    initializeSensorMap();
}

SensorManager::~SensorManager()
{
}

void SensorManager::initializeSensorMap()
{
    sensors_[SensorType::ECG] = {false, 0.0f, ADDR_ECG, "ECG"};
    sensors_[SensorType::SpO2] = {false, 0.0f, ADDR_SPO2, "SpO2"};
    sensors_[SensorType::Temperature] = {false, 0.0f, ADDR_TEMP1, "Temperature"};
    sensors_[SensorType::NIBP] = {false, 0.0f, ADDR_NIBP, "NIBP"};
    sensors_[SensorType::Respiratory] = {true, 0.0f, 0, "Respiratory"}; // Always available (derived)
}

bool SensorManager::initialize()
{
    if (!i2c_->open())
    {
        std::cerr << "[SensorMgr] Failed to open I²C bus" << std::endl;
        if (!mockMode_)
        {
            std::cerr << "[SensorMgr] Continuing in degraded mode..." << std::endl;
        }
        return false;
    }

    std::cout << "[SensorMgr] I²C bus opened successfully" << std::endl;
    
    // Initial sensor scan
    int count = scanSensors();
    std::cout << "[SensorMgr] Found " << count << " sensors" << std::endl;

    return true;
}

int SensorManager::scanSensors()
{
    int count = 0;

    std::cout << "[SensorMgr] Scanning for sensors..." << std::endl;

    for (auto& pair : sensors_)
    {
        SensorType type = pair.first;
        SensorInfo& info = pair.second;

        // Skip respiratory (it's derived from other signals)
        if (type == SensorType::Respiratory)
        {
            continue;
        }

        // Check if sensor responds
        bool present = pingSensor(info.i2cAddress);
        info.attached = present;

        if (present)
        {
            std::cout << "[SensorMgr] ✓ " << info.name << " detected at 0x" 
                      << std::hex << (int)info.i2cAddress << std::dec << std::endl;
            count++;
        }
        else
        {
            std::cout << "[SensorMgr] ✗ " << info.name << " not found at 0x" 
                      << std::hex << (int)info.i2cAddress << std::dec << std::endl;
        }
    }

    return count;
}

bool SensorManager::pingSensor(uint8_t address)
{
    // Send PING command
    if (!i2c_->writeByte(address, CMD_PING))
    {
        return false;
    }

    // Read response
    uint8_t response;
    if (!i2c_->readByte(address, response))
    {
        return false;
    }

    // Check for expected response (0xAA)
    return (response == 0xAA);
}

bool SensorManager::isSensorAttached(SensorType type) const
{
    auto it = sensors_.find(type);
    if (it != sensors_.end())
    {
        return it->second.attached;
    }
    return false;
}

bool SensorManager::readSensor(SensorType type, float& value)
{
    auto it = sensors_.find(type);
    if (it == sensors_.end())
    {
        return false;
    }

    SensorInfo& info = it->second;

    if (!info.attached)
    {
        return false;
    }

    // Skip I²C for respiratory (derived signal)
    if (type == SensorType::Respiratory)
    {
        value = info.lastValue;
        return true;
    }

    // Send READ_DATA command
    if (!i2c_->writeByte(info.i2cAddress, CMD_READ_DATA))
    {
        return false;
    }

    // Read float value (4 bytes)
    if (!i2c_->readFloat(info.i2cAddress, value))
    {
        return false;
    }

    info.lastValue = value;
    return true;
}

const SensorInfo& SensorManager::getSensorInfo(SensorType type) const
{
    static SensorInfo dummy;
    auto it = sensors_.find(type);
    if (it != sensors_.end())
    {
        return it->second;
    }
    return dummy;
}

std::string SensorManager::getSensorStatusJson() const
{
    std::ostringstream json;
    json << "{";
    json << "\"ecg\":" << (isSensorAttached(SensorType::ECG) ? "true" : "false") << ",";
    json << "\"spo2\":" << (isSensorAttached(SensorType::SpO2) ? "true" : "false") << ",";
    json << "\"temp\":" << (isSensorAttached(SensorType::Temperature) ? "true" : "false") << ",";
    json << "\"nibp\":" << (isSensorAttached(SensorType::NIBP) ? "true" : "false") << ",";
    json << "\"resp\":" << (isSensorAttached(SensorType::Respiratory) ? "true" : "false");
    json << "}";
    return json.str();
}
