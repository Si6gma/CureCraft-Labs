#include "webserver.h"
#include "httplib.h"

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <iomanip>

WebServer::WebServer(int port, const std::string& webRoot)
    : port_(port), webRoot_(webRoot), running_(false), updateRateHz_(20)
{
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

    running_ = true;
    
    // Launch server thread
    serverThreadHandle_ = std::make_unique<std::thread>(&WebServer::serverThread, this);
    
    // Launch data streaming thread
    dataThreadHandle_ = std::make_unique<std::thread>(&WebServer::dataStreamThread, this);
    
    std::cout << "🌐 Web Server started on http://localhost:" << port_ << std::endl;
    std::cout << "📂 Serving files from: " << webRoot_ << std::endl;
    std::cout << "🔌 WebSocket endpoint: ws://localhost:" << port_ << "/ws" << std::endl;
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
    
    // Serve static files from web directory
    server.set_mount_point("/", webRoot_);
    
    // WebSocket endpoint for real-time data
    server.Get("/ws", [this](const httplib::Request& req, httplib::Response& res) {
        // Note: cpp-httplib doesn't have built-in WebSocket support
        // For full WebSocket support, we'd need a library like websocketpp
        // For now, we'll use Server-Sent Events (SSE) which works great for one-way streaming
        
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
             << "\"time\":" << std::fixed << std::setprecision(2) << signalGen_.getTime()
             << "}";
        
        res.set_content(json.str(), "application/json");
    });
    
    // API endpoint to set update rate
    server.Post("/api/rate", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            int rate = std::stoi(req.body);
            setUpdateRate(rate);
            res.set_content("{\"success\":true}", "application/json");
        } catch (...) {
            res.set_content("{\"success\":false,\"error\":\"Invalid rate\"}", "application/json");
            res.status = 400;
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
         << "\"timestamp\":" << data.timestamp
         << "}";
    return json.str();
}
