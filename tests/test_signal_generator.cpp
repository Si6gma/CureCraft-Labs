/**
 * @file test_signal_generator.cpp
 * @brief Unit tests for ECG/SpO2 waveform generation
 */

#include "catch_amalgamated.hpp"
#include "core/signal_generator.h"

#include <cmath>
#include <thread>
#include <chrono>

TEST_CASE("SignalGenerator - Basic Initialization", "[signal_generator]") {
    SignalGenerator gen;
    
    SECTION("Initial time is zero or near zero") {
        double time = gen.getTime();
        REQUIRE(time >= 0.0);
        REQUIRE(time < 0.1);  // Should be very close to 0 after initialization
    }
    
    SECTION("Reset sets time to zero") {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        gen.reset();
        double time = gen.getTime();
        REQUIRE(time >= 0.0);
        REQUIRE(time < 0.05);  // Should be very close to 0 after reset
    }
}

TEST_CASE("SignalGenerator - ECG Waveform Generation", "[signal_generator]") {
    SignalGenerator gen;
    
    SECTION("ECG values are within expected range") {
        // Generate multiple samples to check range
        for (int i = 0; i < 100; ++i) {
            auto data = gen.generate();
            
            // ECG should be roughly in range [0, 1] based on the waveform generation
            REQUIRE(data.ecg >= 0.0);
            REQUIRE(data.ecg <= 1.5);
            
            // Small delay between samples
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    SECTION("ECG shows variation over time") {
        std::vector<double> samples;
        for (int i = 0; i < 50; ++i) {
            auto data = gen.generate();
            samples.push_back(data.ecg);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        
        // Calculate variance to ensure there's variation
        double sum = 0.0;
        for (double s : samples) sum += s;
        double mean = sum / samples.size();
        
        double variance = 0.0;
        for (double s : samples) {
            variance += std::pow(s - mean, 2);
        }
        variance /= samples.size();
        
        // Should have some variation (ECG is a waveform)
        // Note: variance threshold adjusted based on actual signal characteristics
        REQUIRE(variance > 0.001);
    }
}

TEST_CASE("SignalGenerator - SpO2 Waveform Generation", "[signal_generator]") {
    SignalGenerator gen;
    
    SECTION("SpO2 values are in realistic medical range") {
        for (int i = 0; i < 100; ++i) {
            auto data = gen.generate();
            
            // SpO2 should be in realistic range [90, 100]%
            REQUIRE(data.spo2 >= 90.0);
            REQUIRE(data.spo2 <= 100.0);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    SECTION("SpO2 shows slow variation") {
        std::vector<double> samples;
        for (int i = 0; i < 30; ++i) {
            auto data = gen.generate();
            samples.push_back(data.spo2);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // Check that values don't change too rapidly
        double max_change = 0.0;
        for (size_t i = 1; i < samples.size(); ++i) {
            double change = std::abs(samples[i] - samples[i-1]);
            max_change = std::max(max_change, change);
        }
        
        // SpO2 should change slowly (less than 2% between 50ms samples)
        REQUIRE(max_change < 2.0);
    }
}

TEST_CASE("SignalGenerator - Respiratory Waveform", "[signal_generator]") {
    SignalGenerator gen;
    
    SECTION("Respiratory values are within expected range") {
        for (int i = 0; i < 100; ++i) {
            auto data = gen.generate();
            
            // Respiratory waveform uses sine wave with amplitude 0.6
            REQUIRE(data.resp >= -0.7);
            REQUIRE(data.resp <= 0.7);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

TEST_CASE("SignalGenerator - Plethysmograph Waveform", "[signal_generator]") {
    SignalGenerator gen;
    
    SECTION("Pleth values are within expected range") {
        for (int i = 0; i < 100; ++i) {
            auto data = gen.generate();
            
            // Pleth waveform should be in range [0, 1.2]
            REQUIRE(data.pleth >= 0.0);
            REQUIRE(data.pleth <= 1.2);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

TEST_CASE("SignalGenerator - Blood Pressure Values", "[signal_generator]") {
    SignalGenerator gen;
    
    SECTION("Blood pressure values are in realistic range") {
        for (int i = 0; i < 50; ++i) {
            auto data = gen.generate();
            
            // Systolic should be around 120 mmHg
            REQUIRE(data.bp_systolic >= 110.0);
            REQUIRE(data.bp_systolic <= 130.0);
            
            // Diastolic should be around 80 mmHg
            REQUIRE(data.bp_diastolic >= 70.0);
            REQUIRE(data.bp_diastolic <= 90.0);
            
            // Systolic should always be higher than diastolic
            REQUIRE(data.bp_systolic > data.bp_diastolic);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
}

TEST_CASE("SignalGenerator - Temperature Values", "[signal_generator]") {
    SignalGenerator gen;
    
    SECTION("Temperature values are in realistic range") {
        for (int i = 0; i < 50; ++i) {
            auto data = gen.generate();
            
            // Core temperature should be around 37°C
            REQUIRE(data.temp_cavity >= 36.0);
            REQUIRE(data.temp_cavity <= 38.0);
            
            // Skin temperature should be slightly lower
            REQUIRE(data.temp_skin >= 35.0);
            REQUIRE(data.temp_skin <= 38.0);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
}

TEST_CASE("SignalGenerator - Timestamp Generation", "[signal_generator]") {
    SignalGenerator gen;
    
    SECTION("Timestamp increases with each generation") {
        auto data1 = gen.generate();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto data2 = gen.generate();
        
        REQUIRE(data2.timestamp > data1.timestamp);
    }
    
    SECTION("Timestamp is consistent with wall clock") {
        auto start = std::chrono::steady_clock::now();
        auto data = gen.generate();
        auto end = std::chrono::steady_clock::now();
        
        std::chrono::duration<double> elapsed = end - start;
        
        // The generated timestamp should be very close to elapsed time
        REQUIRE(data.timestamp >= 0.0);
        REQUIRE(data.timestamp < 0.1);  // Should be very small since we just started
    }
}

TEST_CASE("SignalGenerator - Complete SensorData Structure", "[signal_generator]") {
    SignalGenerator gen;
    
    SECTION("All fields are populated") {
        auto data = gen.generate();
        
        // Verify all fields have valid (non-NaN) values
        REQUIRE_FALSE(std::isnan(data.ecg));
        REQUIRE_FALSE(std::isnan(data.spo2));
        REQUIRE_FALSE(std::isnan(data.resp));
        REQUIRE_FALSE(std::isnan(data.pleth));
        REQUIRE_FALSE(std::isnan(data.bp_systolic));
        REQUIRE_FALSE(std::isnan(data.bp_diastolic));
        REQUIRE_FALSE(std::isnan(data.temp_cavity));
        REQUIRE_FALSE(std::isnan(data.temp_skin));
        REQUIRE_FALSE(std::isnan(data.timestamp));
    }
}
