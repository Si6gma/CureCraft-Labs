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
    Temperature,
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
 * This class handles:
 * - Communication with SAMD21 SensorHub via I²C
 * - Detection of connected sensors through hub
 * - Reading sensor data through hub protocol
 * - Tracking sensor attachment status
 * - Providing sensor data to the application
 */
class SensorManager
{
public:
    /**
     * @brief Construct sensor manager
     * @param mockMode Enable mock mode for testing without hardware
     */
    explicit SensorManager(bool mockMode = false);
    
    ~SensorManager();

    /**
     * @brief Initialize sensor manager and scan for sensors
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Re-scan for sensors (for hot-plug detection)
     * @return Number of sensors detected
     */
    int scanSensors();

    /**
     * @brief Check if sensor is attached
     * @param type Sensor type
     * @return true if attached
     */
    bool isSensorAttached(SensorType type) const;

    /**
     * @brief Read data from sensor via hub
     * @param type Sensor type
     * @param value Output value
     * @return true if successful
     */
    bool readSensor(SensorType type, float& value);

    /**
     * @brief Get sensor info
     * @param type Sensor type
     * @return Sensor information
     */
    const SensorInfo& getSensorInfo(SensorType type) const;

    /**
     * @brief Get all sensor statuses as JSON string  
     * @return JSON string with sensor status
     */
    std::string getSensorStatusJson() const;

private:
    std::unique_ptr<I2CDriver> i2c_;
    std::map<SensorType, SensorInfo> sensors_;
    bool mockMode_;

    void initializeSensorMap();
    SensorId sensorTypeToId(SensorType type) const;
};

#endif // SENSOR_MANAGER_H

