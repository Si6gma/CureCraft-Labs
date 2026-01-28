// MQTTDriver.cpp
#include "core/MQTTDriver.h"

#include <cctype>
#include <cstdlib>
#include <thread>

MQTTDriver::MQTTDriver(std::string serverUri, std::string clientId, SensorDataStore& sensorStore)
: serverUri_(std::move(serverUri))
, clientId_(std::move(clientId))
, client_(serverUri_, clientId_)
, cb_(*this)
, sensorStore_(sensorStore) {
  client_.set_callback(cb_);

  connOpts_.set_clean_session(true);
  connOpts_.set_automatic_reconnect(true);
  connOpts_.set_keep_alive_interval(20);
}

MQTTDriver::~MQTTDriver() {
  try { disconnect(); } catch (...) {}
}

void MQTTDriver::setCredentials(std::string username, std::string password) {
  username_ = std::move(username);
  password_ = std::move(password);
  useAuth_ = true;
}

void MQTTDriver::connect() {
  if (useAuth_) {
    connOpts_.set_user_name(username_);
    connOpts_.set_password(password_);
  }
  client_.connect(connOpts_)->wait();
  subscribeAll_();
}

void MQTTDriver::disconnect() {
  if (client_.is_connected()) {
    client_.disconnect()->wait();
  }
}

bool MQTTDriver::isConnected() const {
  return client_.is_connected();
}

MQTTDriver::PatientData MQTTDriver::getPatientDataSnapshot() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return patient_;
}

void MQTTDriver::setUpdateCallback(UpdateCallback cb) {
  std::lock_guard<std::mutex> lk(mtx_);
  updateCb_ = std::move(cb);
}

void MQTTDriver::Callback::connection_lost(const std::string&) {
  // automatic_reconnect is enabled
}

void MQTTDriver::Callback::message_arrived(mqtt::const_message_ptr msg) {
  owner_.handleMessage_(msg->get_topic(), msg->to_string());
}

void MQTTDriver::subscribeAll_() {
  constexpr int QOS = 0;

  // --- Heart topics (exactly as in your screenshot) ---
  client_.subscribe("heart/heartRate", QOS)->wait();
  client_.subscribe("heart/systolicBP", QOS)->wait();
  client_.subscribe("heart/diastolicBP", QOS)->wait();
  client_.subscribe("heart/strokeVolume", QOS)->wait();
  client_.subscribe("heart/contractility", QOS)->wait();
  client_.subscribe("heart/cardiacOutput", QOS)->wait();
  client_.subscribe("heart/map", QOS)->wait();
  client_.subscribe("heart/prefactor", QOS)->wait();   // NOTE: "prefactor" (lowercase) per screenshot
  client_.subscribe("heart/rhytm", QOS)->wait();       // NOTE: "rhytm" (typo) per screenshot

  // --- Lung topics ---
  client_.subscribe("lung/oxygenSaturation", QOS)->wait();
  client_.subscribe("lung/respiratoryRate", QOS)->wait();
  client_.subscribe("lung/airwayObstruction", QOS)->wait();

  // --- Conditions topics ---
  client_.subscribe("conditions/septic", QOS)->wait();
  client_.subscribe("conditions/anaphylaxis", QOS)->wait();
  client_.subscribe("conditions/diabetesHypo", QOS)->wait();
  client_.subscribe("conditions/diabetsKeto", QOS)->wait(); // NOTE: "diabetsKeto" per screenshot
  client_.subscribe("conditions/cardiacArrest", QOS)->wait();
}

void MQTTDriver::setField_(float& field, bool& hasFlag, float value) {
  field = value;
  hasFlag = true;
}

void MQTTDriver::handleMessage_(const std::string& topic, const std::string& payloadRaw) {
  const std::string payload = trim_(payloadRaw);

  float value = NAN;
  bool parsed = false;

  const bool isCondition = (topic.rfind("conditions/", 0) == 0);
  if (isCondition) {
    parsed = parseBoolish_(payload, value);
    if (!parsed) parsed = parseFloat_(payload, value);
  } else {
    parsed = parseFloat_(payload, value);
  }
  if (!parsed) return;

  UpdateCallback cbCopy;

  // --- Update SensorDataStore where it makes sense ---
  // SensorData fields: ecg, spo2, resp, pleth, bp_systolic, bp_diastolic, temp_cavity, temp_skin, timestamp
  if (topic == "lung/oxygenSaturation") {
    sensorStore_.setSpo2(static_cast<double>(value));
  } else if (topic == "lung/respiratoryRate") {
    sensorStore_.setResp(static_cast<double>(value));
  } else if (topic == "heart/systolicBP") {
    sensorStore_.setBpSystolic(static_cast<double>(value));
  } else if (topic == "heart/diastolicBP") {
    sensorStore_.setBpDiastolic(static_cast<double>(value));
  }
  // Note: no MQTT temp or ECG topics were provided in your list, so we can't set them yet.

  // --- Update MQTTDriver::PatientData for everything (including conditions) ---
  {
    std::lock_guard<std::mutex> lk(mtx_);

    if (topic == "heart/heartRate") {
      setField_(patient_.heartRate, patient_.has_heartRate, value);
    } else if (topic == "heart/strokeVolume") {
      setField_(patient_.strokeVolume, patient_.has_strokeVolume, value);
    } else if (topic == "heart/contractility") {
      setField_(patient_.contractility, patient_.has_contractility, value);
    } else if (topic == "heart/cardiacOutput") {
      setField_(patient_.cardiacOutput, patient_.has_cardiacOutput, value);
    } else if (topic == "heart/map") {
      setField_(patient_.meanArterialPressure, patient_.has_meanArterialPressure, value);
    } else if (topic == "heart/prefactor") {
      setField_(patient_.preFactor, patient_.has_preFactor, value);
    } else if (topic == "heart/rhytm") {
      setField_(patient_.rhythm, patient_.has_rhythm, value);
    }

    else if (topic == "lung/oxygenSaturation") {
      setField_(patient_.oxygenSaturation, patient_.has_oxygenSaturation, value);
    } else if (topic == "lung/respiratoryRate") {
      setField_(patient_.respiratoryRate, patient_.has_respiratoryRate, value);
    } else if (topic == "lung/airwayObstruction") {
      setField_(patient_.airwayObstruction, patient_.has_airwayObstruction, value);
    }

    else if (topic == "conditions/septic") {
      setField_(patient_.septic, patient_.has_septic, value);
    } else if (topic == "conditions/anaphylaxis") {
      setField_(patient_.anaphylaxis, patient_.has_anaphylaxis, value);
    } else if (topic == "conditions/diabetesHypo") {
      setField_(patient_.diabetesHypo, patient_.has_diabetesHypo, value);
    } else if (topic == "conditions/diabetsKeto") {
      setField_(patient_.diabetesKeto, patient_.has_diabetesKeto, value);
    } else if (topic == "conditions/cardiacArrest") {
      setField_(patient_.cardiacArrest, patient_.has_cardiacArrest, value);
    } else if (topic == "heart/systolicBP") {
      // optional: also mirror into patient_ if you want
    } else if (topic == "heart/diastolicBP") {
      // optional
    }

    cbCopy = updateCb_;
  }

  if (cbCopy) cbCopy(topic, value);
}

std::string MQTTDriver::trim_(std::string s) {
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) i++;
  s.erase(0, i);
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
  return s;
}

std::string MQTTDriver::lower_(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

bool MQTTDriver::parseFloat_(const std::string& s, float& out) {
  if (s.empty()) return false;
  const char* c = s.c_str();
  char* end = nullptr;
  out = std::strtof(c, &end);
  if (end == c) return false;
  while (*end) {
    if (!std::isspace(static_cast<unsigned char>(*end))) return false;
    ++end;
  }
  return true;
}

bool MQTTDriver::parseBoolish_(const std::string& sIn, float& out) {
  if (sIn.empty()) return false;
  const std::string s = lower_(trim_(sIn));

  if (s == "true" || s == "on" || s == "yes") { out = 1.0f; return true; }
  if (s == "false" || s == "off" || s == "no") { out = 0.0f; return true; }

  float v = NAN;
  if (parseFloat_(s, v)) { out = (v != 0.0f) ? 1.0f : 0.0f; return true; }
  return false;
}
