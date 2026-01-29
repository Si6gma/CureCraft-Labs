#ifndef MQTTDRIVER_H
#define MQTTDRIVER_H

#include <string>
#include <functional>
#include <mutex>
#include <mosquitto.h>
#include "core/SensorDataStore.h"

/**
 * @brief MQTT client driver for patient simulation telemetry
 * 
 * Subscribes to medical simulation topics (heart, lung, conditions) from an MQTT broker
 * and updates the SensorDataStore with received values.
 */
class MQTTDriver {
public:
    /**
     * @brief Patient data snapshot from MQTT topics
     */
    struct PatientData {
        // Heart metrics
        float heartRate = 0.0f;
        float systolicBP = 0.0f;
        float diastolicBP = 0.0f;
        float strokeVolume = 0.0f;
        float contractility = 0.0f;
        float cardiacOutput = 0.0f;
        float meanArterialPressure = 0.0f;
        float preFactor = 0.0f;
        float rhythm = 0.0f;

        // Lung metrics
        float oxygenSaturation = 0.0f;
        float respiratoryRate = 0.0f;
        float airwayObstruction = 0.0f;

        // Conditions (0.0 = false, 1.0 = true)
        float septic = 0.0f;
        float anaphylaxis = 0.0f;
        float diabetesHypo = 0.0f;
        float diabetesKeto = 0.0f;
        float cardiacArrest = 0.0f;

        // Availability flags
        bool has_heartRate = false;
        bool has_systolicBP = false;
        bool has_diastolicBP = false;
        bool has_strokeVolume = false;
        bool has_contractility = false;
        bool has_cardiacOutput = false;
        bool has_meanArterialPressure = false;
        bool has_preFactor = false;
        bool has_rhythm = false;
        bool has_oxygenSaturation = false;
        bool has_respiratoryRate = false;
        bool has_airwayObstruction = false;
        bool has_septic = false;
        bool has_anaphylaxis = false;
        bool has_diabetesHypo = false;
        bool has_diabetesKeto = false;
        bool has_cardiacArrest = false;
    };

    /**
     * @brief Callback invoked when a topic update is received
     * @param topic MQTT topic name
     * @param value Parsed numeric value
     */
    using UpdateCallback = std::function<void(const std::string& topic, float value)>;

    /**
     * @brief Construct MQTT driver
     * @param sensorStore Reference to global sensor data store
     */
    explicit MQTTDriver(SensorDataStore& sensorStore);
    ~MQTTDriver();

    /**
     * @brief Set MQTT broker address
     * @param host Broker hostname or IP (default: "127.0.0.1")
     * @param port Broker port (default: 1883)
     */
    void setBroker(std::string host, int port = 1883);

    /**
     * @brief Set MQTT client ID
     * @param clientId Unique client identifier
     */
    void setClientId(std::string clientId);

    /**
     * @brief Set authentication credentials
     * @param username MQTT username
     * @param password MQTT password
     */
    void setCredentials(std::string username, std::string password);

    /**
     * @brief Set keep-alive interval
     * @param seconds Keep-alive period in seconds (default: 60)
     */
    void setKeepAlive(int seconds);

    /**
     * @brief Connect to MQTT broker
     * @return true if connection initiated successfully
     */
    bool connect();

    /**
     * @brief Disconnect from broker
     */
    void disconnect();

    /**
     * @brief Check connection status
     * @return true if connected
     */
    bool isConnected() const;

    /**
     * @brief Process MQTT events (must be called regularly in main loop)
     * @param timeout_ms Timeout in milliseconds
     */
    void loop(int timeout_ms = 10);

    /**
     * @brief Get snapshot of patient data
     * @return Current patient data
     */
    PatientData getPatientDataSnapshot() const;

    /**
     * @brief Set callback for topic updates
     * @param cb Callback function
     */
    void setUpdateCallback(UpdateCallback cb);

private:
    // Mosquitto callbacks (static wrappers)
    static void onConnect_(struct mosquitto* mosq, void* userdata, int rc);
    static void onDisconnect_(struct mosquitto* mosq, void* userdata, int rc);
    static void onMessage_(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* msg);

    // Instance callback handlers
    void handleConnect_(int rc);
    void handleMessage_(const std::string& topic, const void* payload, int payloadlen);

    // Subscribe to all topics
    bool subscribeAll_();

    // Field update helper
    void setField_(float& field, bool& hasFlag, float value);

    // Parsing helpers
    static std::string trim_(std::string s);
    static std::string lower_(std::string s);
    static bool parseFloat_(const char* bytes, int len, float& out);
    static bool parseBoolish_(const char* bytes, int len, float& out);

    // MQTT connection state
    struct mosquitto* mosq_ = nullptr;
    std::string host_ = "127.0.0.1";
    int port_ = 1883;
    std::string clientId_ = "curecraft";
    std::string username_;
    std::string password_;
    bool useAuth_ = false;
    int keepAliveSec_ = 60;
    bool connected_ = false;

    // Data store reference
    SensorDataStore& sensorStore_;

    // Internal patient data snapshot
    mutable std::mutex mtx_;
    PatientData patient_;
    UpdateCallback updateCb_;
};

#endif // MQTTDRIVER_H
