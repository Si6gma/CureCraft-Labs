#include <iostream>
#include <csignal>
#include <atomic>
#include "server/webserver.h"
#include "core/MQTTDriver.h"

std::atomic<bool> shutdownRequested(false);

void signalHandler(int signal)
{
    std::cout << "\n🛑 Shutdown signal received..." << std::endl;
    shutdownRequested = true;
}

int main(int argc, char* argv[])
{
    std::cout << "============================================" << std::endl;
    std::cout << "  CureCraft Patient Monitor - Web Server   " << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;

    // Parse command line arguments
    int port = 8080;
    std::string webRoot = "./web";
    bool mockSensors = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else if (arg == "--web-root" && i + 1 < argc) {
            webRoot = argv[++i];
        } else if (arg == "--mock") {
            mockSensors = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --port PORT         HTTP server port (default: 8080)" << std::endl;
            std::cout << "  --web-root DIR      Web assets directory (default: ./web)" << std::endl;
            std::cout << "  --mock              Enable mock sensor mode (no hardware needed)" << std::endl;
            std::cout << "  --help, -h          Show this help message" << std::endl;
            std::cout << std::endl;
            std::cout << "Example:" << std::endl;
            std::cout << "  " << argv[0] << " --port 3000 --mock" << std::endl;
            return 0;
        }
    }

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create and start web server
    WebServer server(port, webRoot, mockSensors);
    server.start();

    auto& store = SensorDataStore::instance();

    MQTTDriver mqtt(store);
    mqtt.setKeepAlive(20);


    mqtt.setBroker("127.0.0.1", 1883);        // change to broker IP if not local
    mqtt.setClientId("curecraft");      // unique client id

    if (!mqtt.connect()) {
        std::cerr << "MQTT connect failed\n";
    }


    std::cout << std::endl;
    std::cout << "✅ Server is running!" << std::endl;
    std::cout << "📱 Open browser to: http://localhost:" << port << std::endl;
    std::cout << "⌨️  Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Keep main thread alive until shutdown requested
    while (!shutdownRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        mqtt.loop(10);
    }

    // Graceful shutdown
    std::cout << "Stopping server..." << std::endl;
    server.stop();
    
    std::cout << "✅ Server stopped cleanly" << std::endl;
    return 0;
}
