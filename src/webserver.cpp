#include "webserver.h"
#include "httplib.h"
#include "auth.h"
#include "sensor_manager.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <iomanip>

WebServer::WebServer(int port, const std::string& webRoot, bool mockSensors)
    : port_(port), webRoot_(webRoot), running_(false), updateRateHz_(20), mockMode_(mockSensors)
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

    // Initialize sensor manager
    if (!sensorMgr_->initialize()) {
        std::cerr << "Warning: Sensor manager initialization failed, continuing in degraded mode" << std::endl;
    }

    running_ = true;
    
    // Launch server thread
    serverThreadHandle_ = std::make_unique<std::thread>(&WebServer::serverThread, this);
    
    // Launch data streaming thread
    dataThreadHandle_ = std::make_unique<std::thread>(&WebServer::dataStreamThread, this);
    
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
    
    running_ = false;
    
    // Wait for threads to finish
    if (serverThreadHandle_ && serverThreadHandle_->joinable()) {
        serverThreadHandle_->join();
    }
    if (dataThreadHandle_ && dataThreadHandle_->joinable()) {
        dataThreadHandle_->join();
    }
    
    std::cout << "Server stopped" << std::endl;
}

void WebServer::setUpdateRate(int hz)
{
    if (hz > 0 && hz <= 120) {
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
    httplib::Server server;
    
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << "Request: " << req.method << " " << req.path << " -> " << res.status << std::endl;
    });

    // Serve static files from web directory (configured later to allow API routes priority)
    
    // Login endpoint
    server.Post("/api/login", [this](const httplib::Request& req, httplib::Response& res) {
        // Parse JSON body (simple parsing for username/password)
        std::string body = req.body;
        
        // Extract username and password (simple parsing, not production-ready!)
        size_t userPos = body.find("\"username\":\"");
        size_t passPos = body.find("\"password\":\"");
        
        if (userPos == std::string::npos || passPos == std::string::npos) {
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
        
        bool valid = Authentication::validateLogin(username, password);
        
        if (valid) {
            res.set_content("{\"success\":true}", "application/json");
        } else {
            res.set_content("{\"success\":false,\"error\":\"Invalid credentials\"}", "application/json");
            res.status = 401;
        }
    });
    
    // Sensor status endpoint
    server.Get("/api/sensors", [this](const httplib::Request& req, httplib::Response& res) {
        std::string json = sensorMgr_->getSensorStatusJson();
        res.set_content(json, "application/json");
    });
    
    // Brightness control endpoint  
    server.Post("/api/brightness", [this](const httplib::Request& req, httplib::Response& res) {
        // In a real implementation, this would control screen brightness
        // For now, just acknowledge the request
        std::cout << "[API] Brightness change requested: " << req.body << std::endl;
        res.set_content("{\"success\":true}", "application/json");
    });
    
    // Server-Sent Events endpoint for real-time data
    server.Get("/ws", [this](const httplib::Request& req, httplib::Response& res) {
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
    server.Get("/api/status", [this](const httplib::Request& req, httplib::Response& res) {
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

    // Serve static files manually to ensure API POST requests aren't shadowed by mount point
    server.Get("/.*", [this, getMimeType](const httplib::Request& req, httplib::Response& res) {
        std::string path = req.path;
        if (path == "/") path = "/index.html";
        
        // Prevent directory traversal
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
            // Only log if not looking for favicon (reduce noise)
            if (path.find("favicon.ico") == std::string::npos) {
                std::cerr << "File not found: " << fullPath << std::endl;
            }
        }
    });
    
    std::cout << "Starting HTTP server on port " << port_ << "..." << std::endl;
    server.listen("0.0.0.0", port_);
}

void WebServer::dataStreamThread()
{
    // This thread manages the signal generator timing
    // The actual data is sent via SSE in the serverThread
    
    while (running_) {
        const int intervalMs = 1000 / updateRateHz_.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
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
