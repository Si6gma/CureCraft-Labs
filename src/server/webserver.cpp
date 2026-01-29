#include "server/webserver.h"
#include "httplib.h"
#include "server/auth.h"
#include "hardware/sensor_manager.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <iomanip>

namespace {
    constexpr int DEFAULT_PORT = 8080;
    constexpr int DEFAULT_UPDATE_RATE_HZ = 20;
    constexpr int MAX_UPDATE_RATE_HZ = 120;
    constexpr int SENSOR_SCAN_INTERVAL_SEC = 3;
}

WebServer::WebServer(int port, const std::string& webRoot, bool mockSensors)
    : port_(port), webRoot_(webRoot), running_(false), updateRateHz_(DEFAULT_UPDATE_RATE_HZ), mockMode_(mockSensors)
{
    // Initialize sensor manager
    sensorMgr_ = std::make_unique<SensorManager>(mockMode_);
}

WebServer::~WebServer()
{
    stop();
}

void WebServer::start()
{
    if (running_) {
        std::cerr << "Server already running" << std::endl;
        return;
    }

    if (!sensorMgr_->initialize()) {
        std::cerr << "Warning: Sensor manager initialization failed, continuing in degraded mode" << std::endl;
    }

    running_ = true;
    
    // Launch server thread
    server_ = std::make_unique<httplib::Server>();
    serverThreadHandle_ = std::make_unique<std::thread>(&WebServer::serverThread, this);
    
    // Launch data streaming thread
    dataThreadHandle_ = std::make_unique<std::thread>(&WebServer::dataStreamThread, this);
    
    sensorScanThreadHandle_ = std::make_unique<std::thread>(&WebServer::sensorScanThread, this);
    
    std::cout << "🌐 Web Server started on http://localhost:" << port_ << std::endl;
    std::cout << "📂 Serving files from: " << webRoot_ << std::endl;
    std::cout << "🔌 Data endpoint: http://localhost:" << port_ << "/ws" << std::endl;
    if (mockMode_) {
        std::cout << "🎭 Mock mode: Sensors simulated" << std::endl;
    }
}

void WebServer::stop()
{
    if (!running_) return;
    
    std::cout << "[WebServer] Stopping..." << std::endl;
    running_ = false;
    shutdownCv_.notify_all();
    
    if (server_) {
        server_->stop();
    }
    
    if (serverThreadHandle_ && serverThreadHandle_->joinable()) {
        serverThreadHandle_->join();
    }
    if (dataThreadHandle_ && dataThreadHandle_->joinable()) {
        dataThreadHandle_->join();
    }
    if (sensorScanThreadHandle_ && sensorScanThreadHandle_->joinable()) {
        sensorScanThreadHandle_->join();
    }
    
    std::cout << "[WebServer] Server stopped cleanly" << std::endl;
}

void WebServer::setUpdateRate(int hz)
{
    if (hz > 0 && hz <= MAX_UPDATE_RATE_HZ) {
        updateRateHz_ = hz;
        std::cout << "Update rate set to " << hz << " Hz" << std::endl;
    }
}

int WebServer::getClientCount() const
{
    std::lock_guard<std::mutex> lock(clientsMutex_);
    return clients_.size();
}

void WebServer::serverThread()
{
    if (!server_) return;
    
    server_->set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << "Request: " << req.method << " " << req.path << " -> " << res.status << std::endl;
    });

    // Enable CORS for all endpoints
    server_->set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    // Handle OPTIONS preflight requests
    server_->Options("/.*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });
    
    
    server_->Post("/api/login", [this](const httplib::Request& req, httplib::Response& res) {
        std::cout << "[API] Login request received from " << req.remote_addr << std::endl;
        std::cout << "[API] Request body: " << req.body << std::endl;
        
        std::string body = req.body;
        
        size_t userPos = body.find("\"username\":\"");
        size_t passPos = body.find("\"password\":\"");
        
        if (userPos == std::string::npos || passPos == std::string::npos) {
            std::cout << "[API] Login failed: Invalid request format" << std::endl;
            res.set_content("{\"success\":false,\"error\":\"Invalid request\"}", "application/json");
            res.status = 400;
            return;
        }
        
        userPos += 12; // Length of "username":"
        passPos += 12; // Length of "password":"
        
        size_t userEnd = body.find("\"", userPos);
        size_t passEnd = body.find("\"", passPos);
        
        std::string username = body.substr(userPos, userEnd - userPos);
        std::string password = body.substr(passPos, passEnd - passPos);
        
        std::cout << "[API] Login attempt - username: " << username << std::endl;
        
        bool valid = Authentication::validateLogin(username, password);
        
        if (valid) {
            std::cout << "[API] Login successful!" << std::endl;
            res.set_content("{\"success\":true}", "application/json");
        } else {
            std::cout << "[API] Login failed: Invalid credentials" << std::endl;
            res.set_content("{\"success\":false,\"error\":\"Invalid credentials\"}", "application/json");
            res.status = 401;
        }
    });
    
    // Logout endpoint
    server_->Post("/api/logout", [this](const httplib::Request& req, httplib::Response& res) {
        std::cout << "[API] Logout request received from " << req.remote_addr << std::endl;
        res.set_content("{\"success\":true}", "application/json");
    });
    
    // Sensor status endpoint
    server_->Get("/api/sensors", [this](const httplib::Request& req, httplib::Response& res) {
        std::string json = sensorMgr_->getSensorStatusJson();
        res.set_content(json, "application/json");
    });
    
    server_->Post("/api/brightness", [this](const httplib::Request& req, httplib::Response& res) {
        std::cout << "[API] Brightness change requested: " << req.body << std::endl;
        res.set_content("{\"success\":true}", "application/json");
    });
    
    // Server-Sent Events endpoint for real-time data
    server_->Get("/ws", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Content-Type", "text/event-stream");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");
        res.set_header("Access-Control-Allow-Origin", "*");
        
        res.set_content_provider(
            "text/event-stream",
            [this](size_t offset, httplib::DataSink& sink) {
                const int intervalMs = 1000 / updateRateHz_.load();
                
                while (running_ && sink.is_writable()) {
                    auto data = signalGen_.generate();
                    std::string json = generateJsonData(data);
                    
                    std::ostringstream oss;
                    oss << "data: " << json << "\n\n";
                    
                    if (!sink.write(oss.str().c_str(), oss.str().size())) {
                        break;
                    }
                    
                    signalGen_.tick(intervalMs / 1000.0);
                    std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
                }
                
                return true;
            }
        );
    });
    
    // API endpoint to get server status
    server_->Get("/api/status", [this](const httplib::Request& req, httplib::Response& res) {
        std::ostringstream json;
        json << "{"
             << "\"running\":true,"
             << "\"clients\":" << getClientCount() << ","
             << "\"updateRate\":" << updateRateHz_.load() << ","
             << "\"time\":" << std::fixed << std::setprecision(2) << signalGen_.getTime() << ","
             << "\"mockMode\":" << (mockMode_ ? "true" : "false")
             << "}";
        
        res.set_content(json.str(), "application/json");
    });
    
    // Helper to determine mime type
    auto getMimeType = [](const std::string& path) -> std::string {
        if (path.find(".html") != std::string::npos) return "text/html";
        if (path.find(".css") != std::string::npos) return "text/css";
        if (path.find(".js") != std::string::npos) return "application/javascript";
        return "text/plain";
    };

    // Serve static files - but NOT for /api paths (let those 404 if not explicitly handled)
    server_->Get("/.*", [this, getMimeType](const httplib::Request& req, httplib::Response& res) {
        std::string path = req.path;
        
        // Skip /api paths - they should be handled by explicit API handlers above
        if (path.find("/api/") == 0) {
            res.status = 404;
            res.set_content("{\"error\":\"API endpoint not found\"}", "application/json");
            return;
        }
        
        if (path == "/") path = "/index.html";
        
        if (path.find("..") != std::string::npos) {
            res.status = 403;
            return;
        }

        std::string fullPath = webRoot_ + path;
        std::ifstream file(fullPath, std::ios::binary);
        
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), getMimeType(path));
        } else {
            res.status = 404;
            if (path.find("favicon.ico") == std::string::npos) {
                std::cerr << "File not found: " << fullPath << std::endl;
            }
        }
    });
    
    std::cout << "Starting HTTP server on port " << port_ << "..." << std::endl;
    server_->listen("0.0.0.0", port_);
}

void WebServer::dataStreamThread()
{
    // This thread manages the signal generator timing
    // The actual data is sent via SSE in the serverThread
    
    while (running_) {
        const int intervalMs = 1000 / updateRateHz_.load();
        
        std::unique_lock<std::mutex> lock(shutdownMutex_);
        if (shutdownCv_.wait_for(lock, std::chrono::milliseconds(intervalMs), [this]{ return !running_; })) {
            break;
        }
    }
}

void WebServer::sensorScanThread()
{
    std::cout << "[WebServer] Sensor hot-plug detection enabled (scans every " << SENSOR_SCAN_INTERVAL_SEC << " seconds)" << std::endl;
    
    while (running_) {
        {
            std::unique_lock<std::mutex> lock(shutdownMutex_);
            if (shutdownCv_.wait_for(lock, std::chrono::seconds(SENSOR_SCAN_INTERVAL_SEC), [this]{ return !running_; })) {
                break;
            }
        }
        
        if (!running_) break;
        
        sensorMgr_->scanSensors();
    }
}

std::string WebServer::generateJsonData(const SignalGenerator::SensorData& data)
{
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);
    json << "{"
         << "\"ecg\":" << data.ecg << ","
         << "\"spo2\":" << data.spo2 << ","
         << "\"resp\":" << data.resp << ","
         << "\"pleth\":" << data.pleth << ","
         << "\"bp_systolic\":" << std::setprecision(1) << data.bp_systolic << ","
         << "\"bp_diastolic\":" << std::setprecision(1) << data.bp_diastolic << ","
         << "\"temp_cavity\":" << std::setprecision(2) << data.temp_cavity << ","
         << "\"temp_skin\":" << std::setprecision(2) << data.temp_skin << ","
         << "\"timestamp\":" << std::setprecision(4) << data.timestamp << ","
         << "\"sensors\":" << sensorMgr_->getSensorStatusJson()
         << "}";
    return json.str();
}
