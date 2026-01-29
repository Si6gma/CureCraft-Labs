// SensorDataStore.h
#pragma once

#include <mutex>
#include <optional>
#include <chrono>

// Adjust include path if your project uses a different include root:
#include "core/signal_generator.h"

class SensorDataStore {
public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;


  static SensorDataStore& instance();


  // ----- Setters -----
  void setEcg(double v);
  void setSpo2(double v);
  void setResp(double v);
  void setPleth(double v);
  void setBpSystolic(double v);
  void setBpDiastolic(double v);
  void setTempCavity(double v);
  void setTempSkin(double v);
  void setTimestamp(double v);

  // Optional convenience: set many at once (only overwrites provided fields)
  void setBulk(const double& ecg,
               const double& spo2,
               const double& resp,
               const double& pleth,
               const double& bp_systolic,
               const double& bp_diastolic,
               const double& temp_cavity,
               const double& temp_skin,
               const double& timestamp);

  // ----- Getters (every value) -----
  double getEcg() const;
  double getSpo2() const;
  double getResp() const;
  double getPleth() const;
  double getBpSystolic() const;
  double getBpDiastolic() const;
  double getTempCavity() const;
  double getTempSkin() const;
  double getTimestamp() const;

  // ----- "Has value" flags -----
  bool hasEcg() const;
  bool hasSpo2() const;
  bool hasResp() const;
  bool hasPleth() const;
  bool hasBpSystolic() const;
  bool hasBpDiastolic() const;
  bool hasTempCavity() const;
  bool hasTempSkin() const;
  bool hasTimestamp() const;

  // ----- Snapshot of the underlying struct -----
  // Note: snapshot returns the struct as-is (fields may be stale if never set).
  SignalGenerator::SensorData snapshot() const;

  // ----- Per-field last update timestamps -----
  TimePoint lastUpdateEcg() const;
  TimePoint lastUpdateSpo2() const;
  TimePoint lastUpdateResp() const;
  TimePoint lastUpdatePleth() const;
  TimePoint lastUpdateBpSystolic() const;
  TimePoint lastUpdateBpDiastolic() const;
  TimePoint lastUpdateTempCavity() const;
  TimePoint lastUpdateTempSkin() const;
  TimePoint lastUpdateTimestamp() const;

private:
    SensorDataStore();

  void setField_(double& field, bool& hasFlag, TimePoint& ts, double v);

  mutable std::mutex mtx_;
  SignalGenerator::SensorData data_{}; // from core/signal_generator.h

  // availability flags
  bool has_ecg_ = false;
  bool has_spo2_ = false;
  bool has_resp_ = false;
  bool has_pleth_ = false;
  bool has_bp_systolic_ = false;
  bool has_bp_diastolic_ = false;
  bool has_temp_cavity_ = false;
  bool has_temp_skin_ = false;
  bool has_timestamp_ = false;

  // last update timestamps
  TimePoint ts_ecg_;
  TimePoint ts_spo2_;
  TimePoint ts_resp_;
  TimePoint ts_pleth_;
  TimePoint ts_bp_systolic_;
  TimePoint ts_bp_diastolic_;
  TimePoint ts_temp_cavity_;
  TimePoint ts_temp_skin_;
  TimePoint ts_timestamp_;
};
