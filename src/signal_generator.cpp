#include "signal_generator.h"

SignalGenerator::SignalGenerator()
{
    // Initialize with default parameters
}

SignalGenerator::SensorData SignalGenerator::generate()
{
    SensorData data;
    data.timestamp = time_;

    // ========================================================================
    // ECG Waveform Generation
    // Simulates a realistic ECG with QRS complex (spike) and baseline
    // ========================================================================
    const double ecgBase = params_.ecgAmplitude * 
                          std::sin(2.0 * M_PI * params_.ecgFreq * time_);
    
    // Generate QRS complex spike (realistic heartbeat spike)
    const double phase = std::fmod(time_, 1.0);
    const double spike = std::exp(-std::pow(phase - 0.12, 2) / 0.0007);
    
    data.ecg = ecgBase + params_.ecgSpikeAmplitude * spike;

    // ========================================================================
    // SpO2 Waveform Generation
    // Simulates blood oxygen saturation pulse
    // ========================================================================
    data.spo2 = params_.spO2Base + 
                params_.spO2Amplitude * std::sin(2.0 * M_PI * params_.spO2Freq * time_);

    // ========================================================================
    // Respiratory Waveform Generation
    // Simulates respiratory rate (breathing)
    // ========================================================================
    data.resp = params_.respAmplitude * 
                std::sin(2.0 * M_PI * params_.respFreq * time_);

    return data;
}

void SignalGenerator::tick(double dt)
{
    time_ += dt;
}

void SignalGenerator::reset()
{
    time_ = 0.0;
}
