#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <memory>
#include <map>
#include <string>
#include "hardware/i2c_driver.h"

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
    uint8_t i2cAddress = 0;
    std::string name;
};

/**
 * @brief Manages sensor detection and data reading from SAMD21 modules
 * 
 * This class handles:
 * - Detection of connected sensors via I²C
 * - Reading sensor data from SAMD21 modules
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
     * @brief Scan I²C bus for connected sensors
     * @return Number of sensors found
     */
    int scanSensors();

    /**
     * @brief Check if sensor is attached
     * @param type Sensor type
     * @return true if attached
     */
    bool isSensorAttached(SensorType type) const;

    /**
     * @brief Read data from sensor
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

    // I²C addresses for each sensor module
    static constexpr uint8_t ADDR_ECG = 0x40;
    static constexpr uint8_t ADDR_SPO2 = 0x41;
    static constexpr uint8_t ADDR_TEMP1 = 0x42;
    static constexpr uint8_t ADDR_TEMP2 = 0x43;
    static constexpr uint8_t ADDR_NIBP = 0x44;

    // I²C commands
    static constexpr uint8_t CMD_PING = 0x01;
    static constexpr uint8_t CMD_READ_DATA = 0x02;
    static constexpr uint8_t CMD_TYPE = 0x03;

    void initializeSensorMap();
    bool pingSensor(uint8_t address);
};

#endif // SENSOR_MANAGER_H
