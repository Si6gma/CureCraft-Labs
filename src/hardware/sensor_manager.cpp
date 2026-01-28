#include "hardware/sensor_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>

SensorManager::SensorManager(bool mockMode)
    : mockMode_(mockMode)
{
    // Pi 400 I2C bus 1 (GPIO 2/3 - where SAMD21 is connected)
    i2c_ = std::make_unique<I2CDriver>(1, mockMode);
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
    
    // Try to detect the hub at 0x08
    std::cout << "[SensorMgr] Scanning for SensorHub..." << std::endl;
    bool hubDetected = i2c_->deviceExists(HUB_I2C_ADDRESS);
    
    if (hubDetected)
    {
        std::cout << "[SensorMgr] ✓ SensorHub detected at 0x" 
                  << std::hex << (int)HUB_I2C_ADDRESS << std::dec << std::endl;
    }
    else
    {
        std::cerr << "[SensorMgr] ✗ SensorHub not detected at 0x" 
                  << std::hex << (int)HUB_I2C_ADDRESS << std::dec << std::endl;
        if (!mockMode_)
        {
            std::cerr << "[SensorMgr] Running without hardware sensors" << std::endl;
        }
        return true;
    }

    // Now probe for individual sensors
    // These are the I2C addresses of sensors connected to the hub
    std::cout << "[SensorMgr] Scanning for sensors..." << std::endl;
    
    const uint8_t ECG_ADDR = 0x40;
    const uint8_t SPO2_ADDR = 0x41;
    const uint8_t TEMP_ADDR = 0x42;
    const uint8_t NIBP_ADDR = 0x43;
    
    sensors_[SensorType::ECG].attached = i2c_->deviceExists(ECG_ADDR);
    sensors_[SensorType::SpO2].attached = i2c_->deviceExists(SPO2_ADDR);
    sensors_[SensorType::Temperature].attached = i2c_->deviceExists(TEMP_ADDR);
    sensors_[SensorType::NIBP].attached = i2c_->deviceExists(NIBP_ADDR);
    sensors_[SensorType::Respiratory].attached = false; // Derived signal
    
    // Count detected sensors
    int count = 0;
    for (const auto& pair : sensors_)
    {
        if (pair.second.attached)
        {
            std::cout << "[SensorMgr] ✓ " << pair.second.name << " detected" << std::endl;
            count++;
        }
        else
        {
            std::cout << "[SensorMgr] ✗ " << pair.second.name << " not detected" << std::endl;
        }
    }
    
    std::cout << "[SensorMgr] Found " << count << " sensor(s)" << std::endl;

    return true;
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
    // Sensors are only for presence detection
    // All data comes from the signal generator
    // Just return true if sensor is attached
    auto it = sensors_.find(type);
    if (it == sensors_.end())
    {
        return false;
    }

    SensorInfo& info = it->second;
    return info.attached;
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
