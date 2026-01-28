#include "hardware/sensor_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>

SensorManager::SensorManager(bool mockMode)
    : mockMode_(mockMode)
{
    // Pi 400 uses I2C bus 3 (GPIO 8/9) for SensorHub
    i2c_ = std::make_unique<I2CDriver>(3, mockMode);
    initializeSensorMap();
}

SensorManager::~SensorManager()
{
}

void SensorManager::initializeSensorMap()
{
    sensors_[SensorType::ECG] = {false, 0.0f, SensorId::ECG, "ECG"};
    sensors_[SensorType::SpO2] = {false, 0.0f, SensorId::SPO2, "SpO2"};
    sensors_[SensorType::Temperature] = {false, 0.0f, SensorId::TEMPERATURE, "Temperature"};
    sensors_[SensorType::NIBP] = {false, 0.0f, SensorId::NIBP, "NIBP"};
    sensors_[SensorType::Respiratory] = {true, 0.0f, SensorId::RESPIRATORY, "Respiratory"}; // Always available (derived)
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
    
    // Ping the hub first
    if (i2c_->pingHub())
    {
        std::cout << "[SensorMgr] ✓ SensorHub detected at 0x" 
                  << std::hex << (int)HUB_I2C_ADDRESS << std::dec << std::endl;
    }
    else
    {
        std::cerr << "[SensorMgr] ✗ SensorHub not responding" << std::endl;
        if (!mockMode_)
        {
            return false;
        }
    }
    
    // Scan for sensors
    int count = scanSensors();
    std::cout << "[SensorMgr] Found " << count << " sensors" << std::endl;

    return true;
}

int SensorManager::scanSensors()
{
    std::cout << "[SensorMgr] Scanning for sensors via hub..." << std::endl;

    // Use hub's SCAN_SENSORS command
    uint8_t statusByte = i2c_->scanSensors();
    
    if (statusByte == 0xFF)
    {
        std::cerr << "[SensorMgr] Failed to scan sensors" << std::endl;
        return 0;
    }

    int count = 0;

    // Update sensor status based on status byte
    sensors_[SensorType::ECG].attached = (statusByte & SensorStatusBits::ECG) != 0;
    sensors_[SensorType::SpO2].attached = (statusByte & SensorStatusBits::SPO2) != 0;
    sensors_[SensorType::Temperature].attached = (statusByte & SensorStatusBits::TEMPERATURE) != 0;
    sensors_[SensorType::NIBP].attached = (statusByte & SensorStatusBits::NIBP) != 0;
    sensors_[SensorType::Respiratory].attached = (statusByte & SensorStatusBits::RESPIRATORY) != 0;

    // Print results
    for (const auto& pair : sensors_)
    {
        const SensorInfo& info = pair.second;
        if (info.attached)
        {
            std::cout << "[SensorMgr] ✓ " << info.name << " detected" << std::endl;
            count++;
        }
        else
        {
            std::cout << "[SensorMgr] ✗ " << info.name << " not found" << std::endl;
        }
    }

    return count;
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

    // Use hub's READ_SENSOR command
    if (!i2c_->readSensor(info.sensorId, value))
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

SensorId SensorManager::sensorTypeToId(SensorType type) const
{
    switch (type)
    {
        case SensorType::ECG: return SensorId::ECG;
        case SensorType::SpO2: return SensorId::SPO2;
        case SensorType::Temperature: return SensorId::TEMPERATURE;
        case SensorType::NIBP: return SensorId::NIBP;
        case SensorType::Respiratory: return SensorId::RESPIRATORY;
        default: return SensorId::ECG;
    }
}
