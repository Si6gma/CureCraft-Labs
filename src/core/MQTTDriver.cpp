// src/core/MQTTDriver.cpp
#include "core/MQTTDriver.h"

#include <cstring>
#include <cstdlib>
#include <cctype>

MQTTDriver::MQTTDriver(SensorDataStore& sensorStore)
: sensorStore_(sensorStore) {
  mosquitto_lib_init();

  // Create client
  mosq_ = mosquitto_new(clientId_.c_str(), true /*clean session*/, this);
  if (!mosq_) {
    throw std::runtime_error("mosquitto_new failed");
  }

  mosquitto_connect_callback_set(mosq_, &MQTTDriver::onConnect_);
  mosquitto_disconnect_callback_set(mosq_, &MQTTDriver::onDisconnect_);
  mosquitto_message_callback_set(mosq_, &MQTTDriver::onMessage_);

  // Auto-reconnect settings (simple + effective)
  mosquitto_reconnect_delay_set(mosq_, 1 /*min*/, 10 /*max*/, true /*exponential*/);
}

MQTTDriver::~MQTTDriver() {
  try { disconnect(); } catch (...) {}
  if (mosq_) {
    mosquitto_destroy(mosq_);
    mosq_ = nullptr;
  }
  mosquitto_lib_cleanup();
}

void MQTTDriver::setBroker(std::string host, int port) {
  host_ = std::move(host);
  port_ = port;
}

void MQTTDriver::setClientId(std::string clientId) {
  clientId_ = std::move(clientId);

  // mosquitto client id is set at mosquitto_new time; easiest safe approach:
  // destroy and recreate client with the new id.
  if (mosq_) {
    mosquitto_destroy(mosq_);
    mosq_ = nullptr;
  }

  mosq_ = mosquitto_new(clientId_.c_str(), true, this);
  if (!mosq_) throw std::runtime_error("mosquitto_new failed");

  mosquitto_connect_callback_set(mosq_, &MQTTDriver::onConnect_);
  mosquitto_disconnect_callback_set(mosq_, &MQTTDriver::onDisconnect_);
  mosquitto_message_callback_set(mosq_, &MQTTDriver::onMessage_);
  mosquitto_reconnect_delay_set(mosq_, 1, 10, true);
}

void MQTTDriver::setCredentials(std::string username, std::string password) {
  username_ = std::move(username);
  password_ = std::move(password);
  useAuth_ = true;
}

void MQTTDriver::setKeepAlive(int seconds) {
  keepAliveSec_ = seconds;
}

bool MQTTDriver::connect() {
  if (connected_) return true;
  if (!mosq_) return false;

  if (useAuth_) {
    mosquitto_username_pw_set(mosq_, username_.c_str(), password_.c_str());
  }

  const int rc = mosquitto_connect(mosq_, host_.c_str(), port_, keepAliveSec_);
  if (rc != MOSQ_ERR_SUCCESS) {
    return false;
  }
  return true;
}

void MQTTDriver::disconnect() {
  if (!mosq_) return;
  mosquitto_disconnect(mosq_);
  connected_ = false;
}

bool MQTTDriver::isConnected() const {
  return connected_;
}

void MQTTDriver::loop(int timeout_ms) {
  if (!mosq_) return;

  // This processes incoming/outgoing packets.
  // It will also handle automatic reconnect attempts if the connection drops.
  const int rc = mosquitto_loop(mosq_, timeout_ms, 1 /*max packets*/);
  if (rc == MOSQ_ERR_NO_CONN) {
    connected_ = false;
    // If you want to explicitly trigger reconnect, you can:
    mosquitto_reconnect(mosq_);
  }
}

MQTTDriver::PatientData MQTTDriver::getPatientDataSnapshot() const {
  std::lock_guard<std::mutex> lk(mtx_);
  return patient_;
}

void MQTTDriver::setUpdateCallback(UpdateCallback cb) {
  std::lock_guard<std::mutex> lk(mtx_);
  updateCb_ = std::move(cb);
}

// ---- callbacks ----

void MQTTDriver::onConnect_(struct mosquitto*, void* userdata, int rc) {
  auto* self = static_cast<MQTTDriver*>(userdata);
  if (self) self->handleConnect_(rc);
}

void MQTTDriver::onDisconnect_(struct mosquitto*, void* userdata, int /*rc*/) {
  auto* self = static_cast<MQTTDriver*>(userdata);
  if (self) self->connected_ = false;
}

void MQTTDriver::onMessage_(struct mosquitto*, void* userdata, const struct mosquitto_message* msg) {
  auto* self = static_cast<MQTTDriver*>(userdata);
  if (!self || !msg || !msg->topic) return;

  self->handleMessage_(msg->topic, msg->payload, msg->payloadlen);
}

void MQTTDriver::handleConnect_(int rc) {
  connected_ = (rc == 0);
  if (connected_) {
    subscribeAll_();
  }
}

bool MQTTDriver::subscribeAll_() {
  if (!mosq_) return false;

  // QoS 0 (telemetry)
  constexpr int QOS = 0;

  bool ok = true;

  // Heart (exact names from your screenshot)
  ok &= (mosquitto_subscribe(mosq_, nullptr, "heart/heartRate", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "heart/systolicBP", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "heart/diastolicBP", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "heart/strokeVolume", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "heart/contractility", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "heart/cardiacOutput", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "heart/map", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "heart/prefactor", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "heart/rhytm", QOS) == MOSQ_ERR_SUCCESS);

  // Lung
  ok &= (mosquitto_subscribe(mosq_, nullptr, "lung/oxygenSaturation", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "lung/respiratoryRate", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "lung/airwayObstruction", QOS) == MOSQ_ERR_SUCCESS);

  // Conditions (exact names from your screenshot)
  ok &= (mosquitto_subscribe(mosq_, nullptr, "conditions/septic", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "conditions/anaphylaxis", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "conditions/diabetesHypo", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "conditions/diabetsKeto", QOS) == MOSQ_ERR_SUCCESS);
  ok &= (mosquitto_subscribe(mosq_, nullptr, "conditions/cardiacArrest", QOS) == MOSQ_ERR_SUCCESS);

  return ok;
}

void MQTTDriver::setField_(float& field, bool& hasFlag, float value) {
  field = value;
  hasFlag = true;
}

void MQTTDriver::handleMessage_(const std::string& topic, const void* payload, int payloadlen) {
  const char* bytes = static_cast<const char*>(payload);
  if (!bytes || payloadlen <= 0) return;

  float value = NAN;
  bool parsed = false;

  const bool isCondition = (topic.rfind("conditions/", 0) == 0);
  if (isCondition) {
    parsed = parseBoolish_(bytes, payloadlen, value);
    if (!parsed) parsed = parseFloat_(bytes, payloadlen, value);
  } else {
    parsed = parseFloat_(bytes, payloadlen, value);
  }
  if (!parsed) return;

  // Update SensorDataStore (SensorData struct) where it fits.
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

  UpdateCallback cbCopy;

  // Update internal snapshot for everything
  {
    std::lock_guard<std::mutex> lk(mtx_);

    // ---- switch-like routing (fast + readable) ----
    // We can’t switch on std::string directly; instead we do if-chain here.
    // If you want, I can convert this to enum+switch.
    if (topic == "heart/heartRate") {
      setField_(patient_.heartRate, patient_.has_heartRate, value);
    } else if (topic == "heart/systolicBP") {
      setField_(patient_.systolicBP, patient_.has_systolicBP, value);
    } else if (topic == "heart/diastolicBP") {
      setField_(patient_.diastolicBP, patient_.has_diastolicBP, value);
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
    } else {
      // unknown topic, ignore
      return;
    }

    cbCopy = updateCb_;
  }

  if (cbCopy) cbCopy(topic, value);
}

// ---- parsing helpers ----

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

bool MQTTDriver::parseFloat_(const char* bytes, int len, float& out) {
  if (!bytes || len <= 0) return false;

  // Copy into a small null-terminated buffer
  char buf[64];
  const int n = (len < static_cast<int>(sizeof(buf) - 1)) ? len : static_cast<int>(sizeof(buf) - 1);
  std::memcpy(buf, bytes, n);
  buf[n] = '\0';

  std::string s = trim_(buf);
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

bool MQTTDriver::parseBoolish_(const char* bytes, int len, float& out) {
  if (!bytes || len <= 0) return false;

  char buf[64];
  const int n = (len < static_cast<int>(sizeof(buf) - 1)) ? len : static_cast<int>(sizeof(buf) - 1);
  std::memcpy(buf, bytes, n);
  buf[n] = '\0';

  std::string s = lower_(trim_(buf));
  if (s.empty()) return false;

  if (s == "true" || s == "on" || s == "yes") { out = 1.0f; return true; }
  if (s == "false" || s == "off" || s == "no") { out = 0.0f; return true; }

  float v = NAN;
  // allow numeric and map to 0/1
  const char* c = s.c_str();
  char* end = nullptr;
  v = std::strtof(c, &end);
  if (end != c) {
    while (*end) {
      if (!std::isspace(static_cast<unsigned char>(*end))) return false;
      ++end;
    }
    out = (v != 0.0f) ? 1.0f : 0.0f;
    return true;
  }

  return false;
}
