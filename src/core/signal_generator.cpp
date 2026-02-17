#include "core/signal_generator.h"

#include "core/SensorDataStore.h"
#include <chrono>

SignalGenerator::SignalGenerator()
{
    // Initialize with default parameters
    // Record start time for wall-clock based generation
    startTime_ = std::chrono::steady_clock::now();
}

SignalGenerator::SensorData SignalGenerator::generate()
{
    // Use wall-clock time instead of accumulated ticks
    // This ensures all client connections see the same waveforms
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - startTime_;
    double time_ = elapsed.count();
    
    SensorData data;
    data.timestamp = time_;

    // ========================================================================
    // ECG Waveform Generation  
    // Realistic ECG with P wave, QRS complex, and T wave
    // ========================================================================
    const double ecgHR = 75.0; // Heart rate in BPM
    const double ecgBeatInterval = 60.0 / ecgHR; // ~0.8 seconds per beat
    double ecgBeatPhase = std::fmod(time_, ecgBeatInterval) / ecgBeatInterval;
    
    double ecgValue = 0.0;
    
    // P wave (atrial depolarization) at 0.0-0.15
    if (ecgBeatPhase < 0.15) {
        double pPhase = ecgBeatPhase / 0.15;
        ecgValue = 0.15 * std::exp(-8.0 * std::pow(pPhase - 0.5, 2));
    }
    // PR segment (AV node delay) at 0.15-0.20
    else if (ecgBeatPhase < 0.20) {
        ecgValue = 0.0;
    }
    // QRS complex (ventricular depolarization) at 0.20-0.30
    else if (ecgBeatPhase < 0.30) {
        double qrsPhase = (ecgBeatPhase - 0.20) / 0.10;
        if (qrsPhase < 0.2) {
            // Q wave (small downward deflection)
            ecgValue = -0.1 * (qrsPhase / 0.2);
        }
        else if (qrsPhase < 0.6) {
            // R wave (tall upward spike)
            double rPhase = (qrsPhase - 0.2) / 0.4;
            ecgValue = -0.1 + 1.2 * std::exp(-std::pow((rPhase - 0.5) * 6.0, 2));
        }
        else {
            // S wave (small downward deflection)
            double sPhase = (qrsPhase - 0.6) / 0.4;
            ecgValue = -0.08 * (1.0 - sPhase);
        }
    }
    // ST segment at 0.30-0.40
    else if (ecgBeatPhase < 0.40) {
        ecgValue = 0.0;
    }
    // T wave (ventricular repolarization) at 0.40-0.70
    else if (ecgBeatPhase < 0.70) {
        double tPhase = (ecgBeatPhase - 0.40) / 0.30;
        ecgValue = 0.3 * std::exp(-8.0 * std::pow(tPhase - 0.5, 2));
    }
    // Return to baseline
    else {
        ecgValue = 0.0;
    }
    
    // Scale to fit chart range and add baseline offset
    data.ecg = SensorDataStore::instance().hasEcg() ? SensorDataStore::instance().getEcg() : 0.5 + ecgValue * 0.4;

    // ========================================================================
    // SpO2 Percentage Generation  
    // SpO2 should be a stable percentage (96-99%), not a waveform
    // ========================================================================
    data.spo2 = SensorDataStore::instance().hasSpo2() ? SensorDataStore::instance().getSpo2() : 
                97.5 + 1.0 * std::sin(2.0 * M_PI * 0.02 * time_);  // Slow variation around 97.5%

    // ========================================================================
    // Respiratory Waveform Generation
    // Simulates respiratory rate (breathing)
    // ========================================================================
    data.resp = params_.respAmplitude * 
                std::sin(2.0 * M_PI * params_.respFreq * time_);

    // ========================================================================
    // Plethysmograph Waveform Generation (Pulse oximetry waveform)
    // Realistic pulsatile waveform with dicrotic notch
    // ========================================================================
    const double HR = 75.0;  // Heart rate in BPM
    const double beatInterval = 60.0 / HR;
    const double beatPhase = std::fmod(time_, beatInterval) / beatInterval;
    
    double plethValue = 0.0;
    if (beatPhase < 0.3) {
        // Rapid systolic upstroke
        double upstrokePhase = beatPhase / 0.3;
        plethValue = std::pow(upstrokePhase, 2.0);
    }
    else if (beatPhase < 0.5) {
        // Dicrotic notch
        double notchPhase = (beatPhase - 0.3) / 0.2;
        plethValue = 1.0 - 0.15 * std::sin(notchPhase * M_PI);
    }
    else {
        // Diastolic decay
        double decayPhase = (beatPhase - 0.5) / 0.5;
        plethValue = (1.0 - 0.15) * std::exp(-3.0 * decayPhase);
    }
    
    // Add small baseline noise for realism
    plethValue += 0.02 * std::sin(2.0 * M_PI * 15.0 * time_);
    data.pleth = plethValue;

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

void SignalGenerator::tick(double /* dt */)
{
    // No longer needed - time is now wall-clock based
    // Kept for API compatibility
}

void SignalGenerator::reset()
{
    // Reset start time to now
    startTime_ = std::chrono::steady_clock::now();
}

double SignalGenerator::getTime() const
{
    // Return wall-clock time since start
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - startTime_;
    return elapsed.count();
}

