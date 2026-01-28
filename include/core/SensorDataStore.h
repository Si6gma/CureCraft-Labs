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

  SensorDataStore();

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
  void setBulk(const std::optional<double>& ecg,
               const std::optional<double>& spo2,
               const std::optional<double>& resp,
               const std::optional<double>& pleth,
               const std::optional<double>& bp_systolic,
               const std::optional<double>& bp_diastolic,
               const std::optional<double>& temp_cavity,
               const std::optional<double>& temp_skin,
               const std::optional<double>& timestamp);

  // ----- Getters (every value) -----
  std::optional<double> getEcg() const;
  std::optional<double> getSpo2() const;
  std::optional<double> getResp() const;
  std::optional<double> getPleth() const;
  std::optional<double> getBpSystolic() const;
  std::optional<double> getBpDiastolic() const;
  std::optional<double> getTempCavity() const;
  std::optional<double> getTempSkin() const;
  std::optional<double> getTimestamp() const;

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
  SensorData snapshot() const;

  // ----- Per-field last update timestamps -----
  std::optional<TimePoint> lastUpdateEcg() const;
  std::optional<TimePoint> lastUpdateSpo2() const;
  std::optional<TimePoint> lastUpdateResp() const;
  std::optional<TimePoint> lastUpdatePleth() const;
  std::optional<TimePoint> lastUpdateBpSystolic() const;
  std::optional<TimePoint> lastUpdateBpDiastolic() const;
  std::optional<TimePoint> lastUpdateTempCavity() const;
  std::optional<TimePoint> lastUpdateTempSkin() const;
  std::optional<TimePoint> lastUpdateTimestamp() const;

private:
  void setField_(double& field, bool& hasFlag, std::optional<TimePoint>& ts, double v);

  mutable std::mutex mtx_;
  SensorData data_{}; // from core/signal_generator.h

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
  std::optional<TimePoint> ts_ecg_;
  std::optional<TimePoint> ts_spo2_;
  std::optional<TimePoint> ts_resp_;
  std::optional<TimePoint> ts_pleth_;
  std::optional<TimePoint> ts_bp_systolic_;
  std::optional<TimePoint> ts_bp_diastolic_;
  std::optional<TimePoint> ts_temp_cavity_;
  std::optional<TimePoint> ts_temp_skin_;
  std::optional<TimePoint> ts_timestamp_;
};
