#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <memory>
#include <map>
#include <string>
#include "hardware/i2c_driver.h"
#include "hardware/i2c_protocol.h"

/**
 * @brief Sensor types supported by the system
 */
enum class SensorType
{
    ECG,
    SpO2,
    TempCore,
    TempSkin,
    NIBP,
    Respiratory
};

/**
 * @brief Sensor connection status and data
 */
struct SensorInfo
{
    bool attached = false;
    float lastValue = 0.0f;
    SensorId sensorId;     // Protocol sensor ID
    std::string name;
};

/**
 * @brief Manages sensor detection and data reading via SAMD21 hub
 * 
 * Handles communication with SAMD21 SensorHub via I²C bus, detection of
 * connected sensors, reading sensor data, and tracking attachment status.
 */
class SensorManager
{
public:
    explicit SensorManager(bool mockMode = false);
    ~SensorManager();

    bool initialize();
    int scanSensors();
    bool isSensorAttached(SensorType type) const;
    bool readSensor(SensorType type, float& value);
    const SensorInfo& getSensorInfo(SensorType type) const;
    std::string getSensorStatusJson() const;

private:
    std::unique_ptr<I2CDriver> i2c_;
    std::map<SensorType, SensorInfo> sensors_;
    bool mockMode_;

    void initializeSensorMap();
    SensorId sensorTypeToId(SensorType type) const;
};

#endif // SENSOR_MANAGER_H

