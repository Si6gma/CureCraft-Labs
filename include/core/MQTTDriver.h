// MQTTDriver.h
#pragma once

#include <string>
#include <mutex>
#include <chrono>
#include <functional>
#include <cmath>

#include <mqtt/async_client.h>

#include "SensorDataStore.h"

class MQTTDriver {
public:
  using Clock = std::chrono::steady_clock;

  // Keep non-SensorData vitals/conditions here (since SensorData doesn't include them)
  struct PatientData {
    // Heart
    float heartRate = NAN;
    float strokeVolume = NAN;
    float contractility = NAN;
    float cardiacOutput = NAN;
    float meanArterialPressure = NAN; // heart/map
    float preFactor = NAN;
    float rhythm = NAN;

    // Lung (oxygenSat and respRate also mirrored into SensorDataStore)
    float oxygenSaturation = NAN;
    float respiratoryRate = NAN;
    float airwayObstruction = NAN;

    // Conditions (0/1 floats)
    float septic = NAN;
    float anaphylaxis = NAN;
    float diabetesHypo = NAN;
    float diabetesKeto = NAN;
    float cardiacArrest = NAN;

    // Has flags
    bool has_heartRate = false;
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

  MQTTDriver(std::string serverUri, std::string clientId, SensorDataStore& sensorStore);
  ~MQTTDriver();

  void setCredentials(std::string username, std::string password);

  void connect();
  void disconnect();
  bool isConnected() const;

  // Snapshot of non-SensorData values (heart/lung/conditions)
  PatientData getPatientDataSnapshot() const;

  void setUpdateCallback(UpdateCallback cb);

private:
  class Callback final : public virtual mqtt::callback {
  public:
    explicit Callback(MQTTDriver& owner) : owner_(owner) {}
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr) override {}
  private:
    MQTTDriver& owner_;
  };

  void subscribeAll_();
  void handleMessage_(const std::string& topic, const std::string& payload);

  static bool parseFloat_(const std::string& s, float& out);
  static bool parseBoolish_(const std::string& s, float& out); // 0/1 floats
  static std::string trim_(std::string s);
  static std::string lower_(std::string s);

  void setField_(float& field, bool& hasFlag, float value);

  std::string serverUri_;
  std::string clientId_;
  std::string username_;
  std::string password_;
  bool useAuth_ = false;

  mqtt::async_client client_;
  mqtt::connect_options connOpts_;
  Callback cb_;

  SensorDataStore& sensorStore_;

  mutable std::mutex mtx_;
  PatientData patient_{};
  UpdateCallback updateCb_;
};
