#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include "core/signal_generator.h"
#include "hardware/sensor_manager.h"
#include "httplib.h"

// Forward declarations
class SensorManager;

/**
 * @brief Lightweight HTTP/WebSocket server for patient monitor data
 * 
 * Serves:
 * - Static HTML/CSS/JS files from web/ directory
 * - Real-time sensor data via WebSocket at /ws
 * - RESTful API endpoints for configuration
 */
class WebServer
{
public:
    /**
     * @brief Construct a new WebServer
     * @param port HTTP server port (default: 8080)
     * @param webRoot Directory containing static web assets (default: "./web")
     * @param mockSensors Enable mock sensor mode for testing (default: false)
     */
    WebServer(int port = 8080, const std::string& webRoot = "./web", bool mockSensors = false);
    
    ~WebServer();

    /**
     * @brief Start the web server (non-blocking)
     * Launches HTTP server and begins streaming sensor data to connected clients
     */
    void start();

    /**
     * @brief Stop the web server
     * Gracefully shuts down all connections and threads
     */
    void stop();

    /**
     * @brief Check if server is running
     * @return true if server is active
     */
    bool isRunning() const { return running_; }

    /**
     * @brief Set update rate for sensor data streaming
     * @param hz Update frequency in Hertz (default: 20Hz)
     */
    void setUpdateRate(int hz);

    /**
     * @brief Get current number of connected WebSocket clients
     * @return Number of active connections
     */
    int getClientCount() const;

private:
    void serverThread();
    void dataStreamThread();
    void sensorScanThread();
    std::string generateJsonData(const SignalGenerator::SensorData& data);

    int port_;
    std::string webRoot_;
    std::atomic<bool> running_;
    std::atomic<int> updateRateHz_;
    bool mockMode_;
    
    SignalGenerator signalGen_;
    std::unique_ptr<SensorManager> sensorMgr_;
    std::unique_ptr<httplib::Server> server_;
    
    std::unique_ptr<std::thread> serverThreadHandle_;
    std::unique_ptr<std::thread> dataThreadHandle_;
    std::unique_ptr<std::thread> sensorScanThreadHandle_;
    
    // Thread-safe client management
    mutable std::mutex clientsMutex_;
    std::vector<void*> clients_; // WebSocket connection handles
};

#endif // WEBSERVER_H
