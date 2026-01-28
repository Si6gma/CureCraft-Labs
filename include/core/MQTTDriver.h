// include/core/MQTTDriver.h
#pragma once

#include <string>
#include <mutex>
#include <functional>
#include <cmath>
#include <cstdint>

#include <mosquitto.h>

#include "core/SensorDataStore.h"

class MQTTDriver {
public:
  struct PatientData {
    // Heart
    float heartRate = NAN;
    float systolicBP = NAN;
    float diastolicBP = NAN;
    float strokeVolume = NAN;
    float contractility = NAN;
    float cardiacOutput = NAN;
    float meanArterialPressure = NAN; // heart/map
    float preFactor = NAN;            // heart/prefactor
    float rhythm = NAN;               // heart/rhytm (typo in topic)

    // Lung
    float oxygenSaturation = NAN;
    float respiratoryRate = NAN;
    float airwayObstruction = NAN;

    // Conditions (0/1)
    float septic = NAN;
    float anaphylaxis = NAN;
    float diabetesHypo = NAN;
    float diabetesKeto = NAN;         // conditions/diabetsKeto (typo in topic)
    float cardiacArrest = NAN;

    // Has flags
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

  using UpdateCallback = std::function<void(const std::string& topic, float value)>;

  explicit MQTTDriver(SensorDataStore& sensorStore);
  ~MQTTDriver();

  // Broker config
  void setBroker(std::string host, int port);
  void setClientId(std::string clientId);

  // Optional auth
  void setCredentials(std::string username, std::string password);

  // Optional keepalive (seconds), default 20
  void setKeepAlive(int seconds);

  // Connect/disconnect
  bool connect();      // returns true if connected (or already connected)
  void disconnect();
  bool isConnected() const;

  // Must be called periodically (e.g. every 10–50ms) to process network traffic.
  // Uses mosquitto_loop() (single-threaded integration).
  void loop(int timeout_ms = 10);

  // Access latest non-SensorData vitals/conditions
  PatientData getPatientDataSnapshot() const;

  void setUpdateCallback(UpdateCallback cb);

private:
  // libmosquitto callbacks (static thunks)
  static void onConnect_(struct mosquitto* mosq, void* userdata, int rc);
  static void onDisconnect_(struct mosquitto* mosq, void* userdata, int rc);
  static void onMessage_(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* msg);

  // Internal handlers
  void handleConnect_(int rc);
  void handleMessage_(const std::string& topic, const void* payload, int payloadlen);

  bool subscribeAll_();

  // Parsing helpers
  static bool parseFloat_(const char* bytes, int len, float& out);
  static bool parseBoolish_(const char* bytes, int len, float& out); // -> 0/1
  static std::string trim_(std::string s);
  static std::string lower_(std::string s);

  void setField_(float& field, bool& hasFlag, float value);

  // Config
  std::string host_ = "127.0.0.1";
  int port_ = 1883;
  std::string clientId_ = "curecraft";
  std::string username_;
  std::string password_;
  bool useAuth_ = false;
  int keepAliveSec_ = 20;

  // State
  struct mosquitto* mosq_ = nullptr;
  bool connected_ = false;

  SensorDataStore& sensorStore_;

  mutable std::mutex mtx_;
  PatientData patient_{};
  UpdateCallback updateCb_;
};
