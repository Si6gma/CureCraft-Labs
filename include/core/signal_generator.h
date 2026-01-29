#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include <cmath>
#include <chrono>

/**
 * @brief Signal generator for medical waveforms (ECG, SpO2, Respiratory)
 * 
 * This class generates realistic medical signal waveforms for demonstration
 * and testing purposes. It's designed to be reusable by both Qt GUI and
 * web server implementations.
 * 
 * Uses wall-clock time to ensure all client connections see synchronized
 * waveforms, preventing speed-up when multiple tabs are open.
 * 
 * NOTE: This is a placeholder for real hardware integration. Replace the
 * generate() methods with actual sensor readings when connecting to hardware.
 */
class SignalGenerator
{
public:
    struct SensorData
    {
        double ecg;           // ECG waveform value
        double spo2;          // SpO2 (blood oxygen) waveform value
        double resp;          // Respiratory waveform value
        double pleth;         // Plethysmograph waveform (SpO2 pulse)
        double bp_systolic;   // Blood pressure systolic (mmHg)
        double bp_diastolic;  // Blood pressure diastolic (mmHg)
        double temp_cavity;   // Core/cavity temperature (°C)
        double temp_skin;     // Skin/surface temperature (°C)
        double timestamp;     // Current time in seconds
    };

    SignalGenerator();

    /**
     * @brief Generate sensor data at the current time point
     * @return SensorData struct containing all waveform values
     */
    SensorData generate();

    /**
     * @brief Advance time by delta seconds (deprecated - now uses wall-clock time)
     * @param dt Time delta in seconds (e.g., 0.05 for 20Hz updates)
     */
    void tick(double dt);

    /**
     * @brief Reset time to zero
     */
    void reset();

    /**
     * @brief Get current simulation time
     * @return Current time in seconds
     */
    double getTime() const;

private:
    // Waveform generation parameters (optimized for realistic medical signals)
    struct WaveformParams
    {
        double ecgFreq = 1.0;            // ECG frequency (Hz) - ~60 BPM
        double ecgAmplitude = 0.08;      // ECG baseline amplitude
        double ecgSpikeAmplitude = 1.25; // QRS complex spike amplitude
        double spO2Freq = 1.2;           // SpO2 frequency (Hz) - ~72 BPM
        double spO2Base = 0.55;          // SpO2 baseline offset
        double spO2Amplitude = 0.4;      // SpO2 oscillation amplitude
        double respFreq = 0.3;           // Respiratory frequency (Hz) - ~18 breaths/min
        double respAmplitude = 0.6;      // Respiratory amplitude
    } params_;

    // Wall-clock start time for synchronized signal generation
    std::chrono::steady_clock::time_point startTime_;
};

#endif // SIGNAL_GENERATOR_H
