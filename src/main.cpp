#include <iostream>
#include <csignal>
#include <unistd.h>
#include <atomic>
#include "server/webserver.h"
#include "core/MQTTDriver.h"

namespace {
    constexpr int DEFAULT_PORT = 8080;
    constexpr const char* DEFAULT_WEB_ROOT = "./web";
    constexpr const char* FALLBACK_WEB_ROOT = "../web";
}

std::atomic<bool> shutdownRequested(false);

void signalHandler(int signal)
{
    const char* msg = "\n🛑 Shutdown signal received...\n";
    write(STDERR_FILENO, msg, 31);
    shutdownRequested = true;
}

int main(int argc, char* argv[])
{
    std::cout << "============================================" << std::endl;
    std::cout << "  CureCraft Patient Monitor - Web Server   " << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;

    int port = DEFAULT_PORT;
    std::string webRoot = DEFAULT_WEB_ROOT;
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

    if (webRoot == DEFAULT_WEB_ROOT && access(webRoot.c_str(), F_OK) != 0) {
        if (access(FALLBACK_WEB_ROOT, F_OK) == 0) {
            webRoot = FALLBACK_WEB_ROOT;
            std::cout << "Notice: Default ./web not found, using ../web" << std::endl;
        }
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

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

    while (!shutdownRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        mqtt.loop(10);
    }

    std::cout << "Stopping server..." << std::endl;
    server.stop();
    
    std::cout << "✅ Server stopped cleanly" << std::endl;
    return 0;
}
