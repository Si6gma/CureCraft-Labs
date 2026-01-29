// SensorDataStore.cpp
#include "core/SensorDataStore.h"

SensorDataStore::SensorDataStore() = default;

void SensorDataStore::setField_(double& field,
                                bool& hasFlag,
                                TimePoint& ts,
                                double v) {
  field = v;
  hasFlag = true;
  ts = Clock::now();
}

// ----- Setters -----
void SensorDataStore::setEcg(double v) {
  std::lock_guard<std::mutex> lk(mtx_);
  setField_(data_.ecg, has_ecg_, ts_ecg_, v);
}

void SensorDataStore::setSpo2(double v) {
  std::lock_guard<std::mutex> lk(mtx_);
  setField_(data_.spo2, has_spo2_, ts_spo2_, v);
}

void SensorDataStore::setResp(double v) {
  std::lock_guard<std::mutex> lk(mtx_);
  setField_(data_.resp, has_resp_, ts_resp_, v);
}

void SensorDataStore::setPleth(double v) {
  std::lock_guard<std::mutex> lk(mtx_);
  setField_(data_.pleth, has_pleth_, ts_pleth_, v);
}

void SensorDataStore::setBpSystolic(double v) {
  std::lock_guard<std::mutex> lk(mtx_);
  setField_(data_.bp_systolic, has_bp_systolic_, ts_bp_systolic_, v);
}

void SensorDataStore::setBpDiastolic(double v) {
  std::lock_guard<std::mutex> lk(mtx_);
  setField_(data_.bp_diastolic, has_bp_diastolic_, ts_bp_diastolic_, v);
}

void SensorDataStore::setTempCavity(double v) {
  std::lock_guard<std::mutex> lk(mtx_);
  setField_(data_.temp_cavity, has_temp_cavity_, ts_temp_cavity_, v);
}

void SensorDataStore::setTempSkin(double v) {
  std::lock_guard<std::mutex> lk(mtx_);
  setField_(data_.temp_skin, has_temp_skin_, ts_temp_skin_, v);
}

void SensorDataStore::setTimestamp(double v) {
  std::lock_guard<std::mutex> lk(mtx_);
  setField_(data_.timestamp, has_timestamp_, ts_timestamp_, v);
}

void SensorDataStore::setBulk(const double& ecg,
                              const double& spo2,
                              const double& resp,
                              const double& pleth,
                              const double& bp_systolic,
                              const double& bp_diastolic,
                              const double& temp_cavity,
                              const double& temp_skin,
                              const double& timestamp) {
  std::lock_guard<std::mutex> lk(mtx_);
  if (ecg)         setField_(data_.ecg,         has_ecg_,         ts_ecg_,         ecg);
  if (spo2)        setField_(data_.spo2,        has_spo2_,        ts_spo2_,        spo2);
  if (resp)        setField_(data_.resp,        has_resp_,        ts_resp_,        resp);
  if (pleth)       setField_(data_.pleth,       has_pleth_,       ts_pleth_,       pleth);
  if (bp_systolic) setField_(data_.bp_systolic, has_bp_systolic_, ts_bp_systolic_, bp_systolic);
  if (bp_diastolic)setField_(data_.bp_diastolic,has_bp_diastolic_,ts_bp_diastolic_, bp_diastolic);
  if (temp_cavity) setField_(data_.temp_cavity, has_temp_cavity_, ts_temp_cavity_, temp_cavity);
  if (temp_skin)   setField_(data_.temp_skin,   has_temp_skin_,   ts_temp_skin_,   temp_skin);
  if (timestamp)   setField_(data_.timestamp,   has_timestamp_,   ts_timestamp_,   timestamp);
}

// ----- Getters -----
double SensorDataStore::getEcg() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return has_ecg_ ? data_.ecg : 0;
}

double SensorDataStore::getSpo2() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return has_spo2_ ? double(data_.spo2) : 0;
}

double SensorDataStore::getResp() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return has_resp_ ? double(data_.resp) : 0;
}

double SensorDataStore::getPleth() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return has_pleth_ ? double(data_.pleth) : 0;
}

double SensorDataStore::getBpSystolic() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return has_bp_systolic_ ? double(data_.bp_systolic) : 0;
}

double SensorDataStore::getBpDiastolic() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return has_bp_diastolic_ ? double(data_.bp_diastolic) : 0;
}

double SensorDataStore::getTempCavity() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return has_temp_cavity_ ? double(data_.temp_cavity) : 0;
}

double SensorDataStore::getTempSkin() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return has_temp_skin_ ? double(data_.temp_skin) : 0;
}

double SensorDataStore::getTimestamp() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return has_timestamp_ ? double(data_.timestamp) : 0;
}

// ----- Has flags -----
bool SensorDataStore::hasEcg() const { std::lock_guard<std::mutex> lk(mtx_); return has_ecg_; }
bool SensorDataStore::hasSpo2() const { std::lock_guard<std::mutex> lk(mtx_); return has_spo2_; }
bool SensorDataStore::hasResp() const { std::lock_guard<std::mutex> lk(mtx_); return has_resp_; }
bool SensorDataStore::hasPleth() const { std::lock_guard<std::mutex> lk(mtx_); return has_pleth_; }
bool SensorDataStore::hasBpSystolic() const { std::lock_guard<std::mutex> lk(mtx_); return has_bp_systolic_; }
bool SensorDataStore::hasBpDiastolic() const { std::lock_guard<std::mutex> lk(mtx_); return has_bp_diastolic_; }
bool SensorDataStore::hasTempCavity() const { std::lock_guard<std::mutex> lk(mtx_); return has_temp_cavity_; }
bool SensorDataStore::hasTempSkin() const { std::lock_guard<std::mutex> lk(mtx_); return has_temp_skin_; }
bool SensorDataStore::hasTimestamp() const { std::lock_guard<std::mutex> lk(mtx_); return has_timestamp_; }

// ----- Snapshot -----
SignalGenerator::SensorData SensorDataStore::snapshot() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return data_;
}

// ----- Last update times -----
SensorDataStore::TimePoint SensorDataStore::lastUpdateEcg() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return ts_ecg_;
}
SensorDataStore::TimePoint SensorDataStore::lastUpdateSpo2() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return ts_spo2_;
}
SensorDataStore::TimePoint SensorDataStore::lastUpdateResp() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return ts_resp_;
}
SensorDataStore::TimePoint SensorDataStore::lastUpdatePleth() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return ts_pleth_;
}
SensorDataStore::TimePoint SensorDataStore::lastUpdateBpSystolic() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return ts_bp_systolic_;
}
SensorDataStore::TimePoint SensorDataStore::lastUpdateBpDiastolic() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return ts_bp_diastolic_;
}
SensorDataStore::TimePoint SensorDataStore::lastUpdateTempCavity() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return ts_temp_cavity_;
}
SensorDataStore::TimePoint SensorDataStore::lastUpdateTempSkin() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return ts_temp_skin_;
}
SensorDataStore::TimePoint SensorDataStore::lastUpdateTimestamp() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return ts_timestamp_;
}
SensorDataStore& SensorDataStore::instance() {
  static SensorDataStore inst;
  return inst;
}
