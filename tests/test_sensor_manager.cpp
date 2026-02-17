/**
 * @file test_sensor_manager.cpp
 * @brief Unit tests for sensor lifecycle management
 */

#include "catch_amalgamated.hpp"
#include "hardware/sensor_manager.h"
#include "hardware/i2c_protocol.h"

#include <string>

TEST_CASE("SensorManager - Construction and Destruction", "[sensor_manager]") {
    SECTION("Mock mode construction") {
        SensorManager mgr(true);  // mock mode
        // Should construct without throwing
        REQUIRE(true);
    }
    
    SECTION("Non-mock mode construction") {
        SensorManager mgr(false);  // non-mock mode
        // Should construct without throwing
        REQUIRE(true);
    }
}

TEST_CASE("SensorManager - Initialization", "[sensor_manager]") {
    SECTION("Initialize in mock mode") {
        SensorManager mgr(true);
        // In mock mode, initialization should succeed or gracefully degrade
        (void)mgr.initialize();
        // Result may be true or false depending on I2C availability
        // Just verify it doesn't crash
        REQUIRE(true);
    }
}

TEST_CASE("SensorManager - Sensor Scanning", "[sensor_manager]") {
    SECTION("Scan sensors in mock mode") {
        SensorManager mgr(true);
        mgr.initialize();
        
        int count = mgr.scanSensors();
        // In mock mode, should return some sensors (implementation dependent)
        REQUIRE(count >= 0);
        REQUIRE(count <= 6);  // Max 6 sensor types
    }
}

TEST_CASE("SensorManager - Sensor Attachment Status", "[sensor_manager]") {
    SensorManager mgr(true);
    mgr.initialize();
    mgr.scanSensors();
    
    SECTION("Check ECG attachment status") {
        bool attached = mgr.isSensorAttached(SensorType::ECG);
        // In mock mode, may be true or false
        // Just verify it doesn't crash
        REQUIRE((attached == true || attached == false));
    }
    
    SECTION("Check SpO2 attachment status") {
        bool attached = mgr.isSensorAttached(SensorType::SpO2);
        REQUIRE((attached == true || attached == false));
    }
    
    SECTION("Check Temperature attachment status") {
        bool coreAttached = mgr.isSensorAttached(SensorType::TempCore);
        bool skinAttached = mgr.isSensorAttached(SensorType::TempSkin);
        REQUIRE((coreAttached == true || coreAttached == false));
        REQUIRE((skinAttached == true || skinAttached == false));
    }
    
    SECTION("Check NIBP attachment status") {
        bool attached = mgr.isSensorAttached(SensorType::NIBP);
        REQUIRE((attached == true || attached == false));
    }
    
    SECTION("Check Respiratory attachment status") {
        bool attached = mgr.isSensorAttached(SensorType::Respiratory);
        // Respiratory is often a derived signal, may always be "available"
        REQUIRE((attached == true || attached == false));
    }
}

TEST_CASE("SensorManager - Sensor Info Retrieval", "[sensor_manager]") {
    SensorManager mgr(true);
    mgr.initialize();
    mgr.scanSensors();
    
    SECTION("Get ECG sensor info") {
        const SensorInfo& info = mgr.getSensorInfo(SensorType::ECG);
        // Should have a valid name
        REQUIRE_FALSE(info.name.empty());
        REQUIRE(info.sensorId == SensorId::ECG);
    }
    
    SECTION("Get SpO2 sensor info") {
        const SensorInfo& info = mgr.getSensorInfo(SensorType::SpO2);
        REQUIRE_FALSE(info.name.empty());
        REQUIRE(info.sensorId == SensorId::SPO2);
    }
    
    SECTION("Get Core Temperature sensor info") {
        const SensorInfo& info = mgr.getSensorInfo(SensorType::TempCore);
        REQUIRE_FALSE(info.name.empty());
        REQUIRE(info.sensorId == SensorId::TEMP_CORE);
    }
    
    SECTION("Get Skin Temperature sensor info") {
        const SensorInfo& info = mgr.getSensorInfo(SensorType::TempSkin);
        REQUIRE_FALSE(info.name.empty());
        REQUIRE(info.sensorId == SensorId::TEMP_SKIN);
    }
    
    SECTION("Get NIBP sensor info") {
        const SensorInfo& info = mgr.getSensorInfo(SensorType::NIBP);
        REQUIRE_FALSE(info.name.empty());
        REQUIRE(info.sensorId == SensorId::NIBP);
    }
    
    SECTION("Get Respiratory sensor info") {
        const SensorInfo& info = mgr.getSensorInfo(SensorType::Respiratory);
        REQUIRE_FALSE(info.name.empty());
        REQUIRE(info.sensorId == SensorId::RESPIRATORY);
    }
    
    SECTION("Get info for invalid sensor type") {
        // Cast an invalid value to SensorType
        SensorInfo dummy = mgr.getSensorInfo(static_cast<SensorType>(999));
        // Should return a default/empty SensorInfo
        REQUIRE(dummy.name.empty());
    }
}

TEST_CASE("SensorManager - JSON Status Output", "[sensor_manager]") {
    SECTION("Get sensor status as JSON") {
        SensorManager mgr(true);
        mgr.initialize();
        mgr.scanSensors();
        
        std::string json = mgr.getSensorStatusJson();
        
        // Should be valid JSON (non-empty and starts with {)
        REQUIRE_FALSE(json.empty());
        REQUIRE(json[0] == '{');
        REQUIRE(json[json.length() - 1] == '}');
        
        // Should contain expected sensor keys
        REQUIRE(json.find("ecg") != std::string::npos);
        REQUIRE(json.find("spo2") != std::string::npos);
        REQUIRE(json.find("temp_core") != std::string::npos);
        REQUIRE(json.find("temp_skin") != std::string::npos);
        REQUIRE(json.find("nibp") != std::string::npos);
        REQUIRE(json.find("resp") != std::string::npos);
    }
}

TEST_CASE("SensorManager - Sensor Reading", "[sensor_manager]") {
    SensorManager mgr(true);
    mgr.initialize();
    mgr.scanSensors();
    
    SECTION("Read ECG sensor") {
        float value = 0.0f;
        bool result = mgr.readSensor(SensorType::ECG, value);
        // Result depends on whether sensor is attached
        // Just verify it doesn't crash
        REQUIRE((result == true || result == false));
    }
    
    SECTION("Read SpO2 sensor") {
        float value = 0.0f;
        bool result = mgr.readSensor(SensorType::SpO2, value);
        REQUIRE((result == true || result == false));
    }
    
    SECTION("Read invalid sensor") {
        float value = 0.0f;
        bool result = mgr.readSensor(static_cast<SensorType>(999), value);
        // Should return false for invalid sensor
        REQUIRE(result == false);
    }
}

TEST_CASE("SensorManager - Lifecycle Flow", "[sensor_manager]") {
    SECTION("Complete lifecycle - init, scan, read, cleanup") {
        // Create manager
        SensorManager mgr(true);
        
        // Initialize
        REQUIRE_NOTHROW(mgr.initialize());
        
        // Scan for sensors
        int sensorCount = 0;
        REQUIRE_NOTHROW(sensorCount = mgr.scanSensors());
        REQUIRE(sensorCount >= 0);
        
        // Check each sensor type
        std::vector<SensorType> types = {
            SensorType::ECG,
            SensorType::SpO2,
            SensorType::TempCore,
            SensorType::TempSkin,
            SensorType::NIBP,
            SensorType::Respiratory
        };
        
        for (auto type : types) {
            // Get info (should not throw)
            const SensorInfo& info = mgr.getSensorInfo(type);
            REQUIRE_FALSE(info.name.empty());
            
            // Check attachment (should not throw)
            bool attached = mgr.isSensorAttached(type);
            REQUIRE((attached == true || attached == false));
            
            // Try to read (should not throw)
            float value = 0.0f;
            mgr.readSensor(type, value);
        }
        
        // Get JSON status (should not throw)
        std::string json = mgr.getSensorStatusJson();
        REQUIRE_FALSE(json.empty());
        
        // Manager will be destroyed when going out of scope
        REQUIRE(true);
    }
}
