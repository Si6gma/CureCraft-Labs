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

    // ========================================================================
    // Plethysmograph Waveform Generation (SpO2 pulse waveform)
    // Simulates the pulse oximetry waveform
    // ========================================================================
    data.pleth = 0.5 + 0.3 * std::sin(2.0 * M_PI * params_.spO2Freq * time_);

    // ========================================================================
    // Blood Pressure Generation
    // Simulates realistic BP values with slow variation
    // ========================================================================
    const double bpVariation = 5.0 * std::sin(2.0 * M_PI * 0.02 * time_); // Slow drift
    data.bp_systolic = 120.0 + bpVariation;
    data.bp_diastolic = 80.0 + bpVariation * 0.5;

    // ========================================================================
    // Temperature Generation
    // Simulates body temperature with slow drift for realism
    // ========================================================================
    const double tempDrift = 0.2 * std::sin(2.0 * M_PI * 0.01 * time_);
    data.temp_cavity = 37.2 + tempDrift;      // Core temperature
    data.temp_skin = 36.8 + tempDrift * 0.8;  // Skin temperature (follows core but dampened)

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
