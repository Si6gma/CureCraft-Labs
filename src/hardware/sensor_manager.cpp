#include "hardware/sensor_manager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <bitset>
#include <thread>
#include <chrono>

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
    sensors_[SensorType::TempCore] = {false, 0.0f, SensorId::TEMP_CORE, "Core Temp"};
    sensors_[SensorType::TempSkin] = {false, 0.0f, SensorId::TEMP_SKIN, "Skin Temp"};
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
    std::cout.flush();
    
    bool hubDetected = i2c_->deviceExists(HUB_I2C_ADDRESS);
    
    if (hubDetected)
    {
        std::cout << "[SensorMgr] ✓ SensorHub detected at 0x" 
                  << std::hex << (int)HUB_I2C_ADDRESS << std::dec << std::endl;
        std::cout.flush();
        
        // Scan for individual sensors using hub protocol
        int count = scanSensors();
        std::cout << "[SensorMgr] Found " << count << " sensor(s)" << std::endl;
        std::cout.flush();
    }
    else
    {
        std::cerr << "[SensorMgr] ✗ SensorHub not detected at 0x" 
                  << std::hex << (int)HUB_I2C_ADDRESS << std::dec << std::endl;
        std::cerr.flush();
        if (!mockMode_)
        {
            std::cerr << "[SensorMgr] Running without hardware sensors" << std::endl;
            std::cerr.flush();
        }
    }

    return true;
}

int SensorManager::scanSensors()
{
    std::cout << "[SensorMgr] Requesting sensor scan from hub..." << std::endl;
    std::cout.flush();
    
    // Send SCAN command (0x01) to hub
    const uint8_t CMD_SCAN = 0x01;
    if (!i2c_->writeByte(HUB_I2C_ADDRESS, CMD_SCAN))
    {
        std::cerr << "[SensorMgr] Failed to send SCAN command" << std::endl;
        std::cerr.flush();
        return 0;
    }
    
    // Give the hub time to perform the scan
    // The hub takes some time to probe multiple I2C buses and print debug info. 
    // Wait longer (500ms) to ensure scan is complete before reading.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Read status byte from hub with validation
    uint8_t statusByte = 0xFF;
    bool success = false;
    int retries = 3;

    while (retries-- > 0)
    {
        if (i2c_->readByte(HUB_I2C_ADDRESS, statusByte))
        {
            // 0xFF usually means the bus is floating high (no response/ACK but read continued)
            // Valid status should not be 0xFF (unless literally all bits including unused ones are set, which is impossible here)
            if (statusByte != 0xFF)
            {
                success = true;
                break;
            }
            std::cerr << "[SensorMgr] Warning: Received 0xFF status (bus idle?), retrying..." << std::endl;
        }
        else
        {
            std::cerr << "[SensorMgr] Failed to read byte, retrying..." << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!success || statusByte == 0xFF)
    {
        std::cerr << "[SensorMgr] Failed to read valid scan results (got 0x" 
                  << std::hex << (int)statusByte << std::dec << ")" << std::endl;
        std::cerr.flush();
        return 0;
    }
    
    std::cout << "[SensorMgr] Status byte: 0b" << std::bitset<8>(statusByte) << std::endl;
    std::cout.flush();
    
    // Parse status byte according to firmware bit assignments
    using namespace SensorStatusBits;
    
    bool ecgDetected = (statusByte & ECG) != 0;
    bool spo2Detected = (statusByte & SPO2) != 0;
    bool coreTempDetected = (statusByte & TEMP_CORE) != 0;
    bool nibpDetected = (statusByte & NIBP) != 0;
    bool skinTempDetected = (statusByte & TEMP_SKIN) != 0;
    
    // Update sensor attachment status
    sensors_[SensorType::ECG].attached = ecgDetected;
    sensors_[SensorType::SpO2].attached = spo2Detected;
    sensors_[SensorType::TempCore].attached = coreTempDetected;
    sensors_[SensorType::TempSkin].attached = skinTempDetected;
    sensors_[SensorType::NIBP].attached = nibpDetected;
    sensors_[SensorType::Respiratory].attached = false;  // Derived signal, not a physical sensor
    
    // Count and report detected sensors
    int count = 0;
    
    if (ecgDetected) {
        std::cout << "[SensorMgr] ✓ ECG detected (0x40)" << std::endl;
        count++;
    } else {
        std::cout << "[SensorMgr] ✗ ECG not detected" << std::endl;
    }
    
    if (spo2Detected) {
        std::cout << "[SensorMgr] ✓ SpO2 detected (0x41)" << std::endl;
        count++;
    } else {
        std::cout << "[SensorMgr] ✗ SpO2 not detected" << std::endl;
    }
    
    if (coreTempDetected) {
        std::cout << "[SensorMgr] ✓ Core Temp detected (W1 0x68)" << std::endl;
        count++;
    } else {
        std::cout << "[SensorMgr] ✗ Core Temp not detected" << std::endl;
    }

    if (skinTempDetected) {
        std::cout << "[SensorMgr] ✓ Skin Temp detected (W2 0x68)" << std::endl;
        count++;
    } else {
        std::cout << "[SensorMgr] ✗ Skin Temp not detected" << std::endl;
    }
    
    if (nibpDetected) {
        std::cout << "[SensorMgr] ✓ NIBP detected (0x43)" << std::endl;
        count++;
    } else {
        std::cout << "[SensorMgr] ✗ NIBP not detected" << std::endl;
    }
    
    std::cout.flush();
    
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
    json << "\"temp_core\":" << (isSensorAttached(SensorType::TempCore) ? "true" : "false") << ",";
    json << "\"temp_skin\":" << (isSensorAttached(SensorType::TempSkin) ? "true" : "false") << ",";
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
        case SensorType::TempCore: return SensorId::TEMP_CORE;
        case SensorType::TempSkin: return SensorId::TEMP_SKIN;
        case SensorType::NIBP: return SensorId::NIBP;
        case SensorType::Respiratory: return SensorId::RESPIRATORY;
        default: return SensorId::ECG;
    }
}
