/**
 * CureCraft SensorHub Firmware
 * 
 * SAMD21-based I2C sensor hub acting as I2C slave to Raspberry Pi 400
 * and I2C master to multiple sensor modules
 * 
 * Hardware Setup:
 *   - W0 (Backbone): I2C slave to Pi 400 at address 0x08
 *     - Pi GPIO 8 (SDA) -> SAMD21 PA23 (SDA)
 *     - Pi GPIO 9 (SCL) -> SAMD21 PA22 (SCL)
 *     - Common GND
 * 
 *   - W1 (Sensors A): I2C master to ECG, SpO2 sensors
 *   - W2 (Sensors B): I2C master to Temperature, NIBP sensors
 */

#include <Wire.h>
#include "protocol.h"

// ============================================================================
// Pin Definitions
// ============================================================================

// LED indicator
#define LED_PIN 14

// I2C pin definitions (SAMD21 specific)
#define W0_SCL 27  // PA22 - Backbone to Pi
#define W0_SDA 26  // PA23 - Backbone to Pi

#define W1_SCL 39  // PA13 - Sensors A
#define W1_SDA 28  // PA12 - Sensors A

#define W2_SCL 13  // PA17 - Sensors B
#define W2_SDA 11  // PA16 - Sensors B

// ============================================================================
// TwoWire Instances
// ============================================================================

// Backbone: I2C slave to Raspberry Pi
TwoWire WireBackbone(&sercom5, W0_SDA, W0_SCL);

// Sensor buses: I2C master to sensor modules
TwoWire WireSensorA(&sercom1, W1_SDA, W1_SCL);
TwoWire WireSensorB(&sercom4, W2_SDA, W2_SCL);

// ============================================================================
// Global State
// ============================================================================

// Command buffer
volatile uint8_t cmdBuffer[8];
volatile uint8_t cmdLength = 0;
volatile bool cmdReady = false;

// Response buffer
uint8_t responseBuffer[8];
uint8_t responseLength = 0;
volatile uint8_t responseIndex = 0;

// Sensor status cache
struct SensorStatus {
    bool attached;
    float lastValue;
};

SensorStatus sensors[5] = {
    {false, 0.0f},  // ECG
    {false, 0.0f},  // SpO2
    {false, 0.0f},  // Temperature
    {false, 0.0f},  // NIBP
    {true, 0.0f}    // Respiratory (always available)
};

// ============================================================================
// Helper Functions
// ============================================================================

void blinkLED(int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);
        delay(50);
    }
}

// Check if sensor exists on I2C bus
bool pingSensor(TwoWire& wire, uint8_t address) {
    wire.beginTransmission(address);
    return (wire.endTransmission() == 0);
}

// Read float from sensor (assumes sensor sends 4 bytes little-endian)
bool readSensorFloat(TwoWire& wire, uint8_t address, float& value) {
    // For now, return mock data
    // In real implementation, send command to sensor and read response
    value = 0.0f;
    return true;
}

// ============================================================================
// Command Processing
// ============================================================================

void scanAllSensors() {
    Serial.println("[Hub] Scanning sensors...");
    
    // Scan sensors on W1 bus (ECG, SpO2)
    sensors[SENSOR_ECG].attached = pingSensor(WireSensorA, ADDR_ECG);
    sensors[SENSOR_SPO2].attached = pingSensor(WireSensorA, ADDR_SPO2);
    
    // Scan sensors on W2 bus (Temperature, NIBP)
    sensors[SENSOR_TEMP].attached = pingSensor(WireSensorB, ADDR_TEMP);
    sensors[SENSOR_NIBP].attached = pingSensor(WireSensorB, ADDR_NIBP);
    
    // Respiratory is always available (derived)
    sensors[SENSOR_RESPIRATORY].attached = true;
    
    // Print results
    Serial.print("  ECG: "); Serial.println(sensors[SENSOR_ECG].attached ? "YES" : "NO");
    Serial.print("  SpO2: "); Serial.println(sensors[SENSOR_SPO2].attached ? "YES" : "NO");
    Serial.print("  Temp: "); Serial.println(sensors[SENSOR_TEMP].attached ? "YES" : "NO");
    Serial.print("  NIBP: "); Serial.println(sensors[SENSOR_NIBP].attached ? "YES" : "NO");
}

uint8_t buildStatusByte() {
    uint8_t status = 0;
    if (sensors[SENSOR_ECG].attached) status |= STATUS_BIT_ECG;
    if (sensors[SENSOR_SPO2].attached) status |= STATUS_BIT_SPO2;
    if (sensors[SENSOR_TEMPERATURE].attached) status |= STATUS_BIT_TEMPERATURE;
    if (sensors[SENSOR_NIBP].attached) status |= STATUS_BIT_NIBP;
    if (sensors[SENSOR_RESPIRATORY].attached) status |= STATUS_BIT_RESPIRATORY;
    return status;
}

void processCommand() {
    if (!cmdReady || cmdLength == 0) return;
    
    uint8_t cmd = cmdBuffer[0];
    
    Serial.print("[Hub] Command: 0x");
    Serial.println(cmd, HEX);
    
    switch (cmd) {
        case CMD_PING:
            // PING: Respond with 0xAA
            responseBuffer[0] = PING_RESPONSE;
            responseLength = 1;
            Serial.println("[Hub] -> PING_RESPONSE");
            break;
            
        case CMD_READ_SENSOR:
            // READ_SENSOR: Read sensor value
            if (cmdLength >= 2) {
                uint8_t sensorId = cmdBuffer[1];
                
                if (sensorId < 5 && sensors[sensorId].attached) {
                    // For now, generate mock data
                    // In real implementation, read from actual sensor
                    float value = 0.0f;
                    
                    switch (sensorId) {
                        case SENSOR_ECG:
                            value = 1.0f + 0.2f * sin(millis() / 1000.0f);
                            break;
                        case SENSOR_SPO2:
                            value = 97.0f;
                            break;
                        case SENSOR_TEMPERATURE:
                            value = 37.2f;
                            break;
                        case SENSOR_NIBP:
                            value = 120.0f;
                            break;
                        case SENSOR_RESPIRATORY:
                            value = 16.0f;
                            break;
                    }
                    
                    // Convert float to bytes (little-endian)
                    memcpy(responseBuffer, &value, sizeof(float));
                    responseLength = 4;
                    
                    Serial.print("[Hub] -> Sensor ");
                    Serial.print(sensorId);
                    Serial.print(": ");
                    Serial.println(value);
                } else {
                    // Sensor not attached or invalid ID
                    responseBuffer[0] = ERROR_RESPONSE;
                    responseLength = 1;
                    Serial.println("[Hub] -> ERROR (sensor not found)");
                }
            }
            break;
            
        case CMD_SCAN_SENSORS:
            // SCAN_SENSORS: Scan all sensors and return status byte
            scanAllSensors();
            responseBuffer[0] = buildStatusByte();
            responseLength = 1;
            Serial.print("[Hub] -> Status: 0x");
            Serial.println(responseBuffer[0], HEX);
            break;
            
        case CMD_GET_STATUS:
            // GET_STATUS: Return detailed status (5 bytes, one per sensor)
            for (int i = 0; i < 5; i++) {
                responseBuffer[i] = sensors[i].attached ? 1 : 0;
            }
            responseLength = 5;
            Serial.println("[Hub] -> Detailed status");
            break;
            
        default:
            // Unknown command
            responseBuffer[0] = ERROR_RESPONSE;
            responseLength = 1;
            Serial.println("[Hub] -> ERROR (unknown command)");
            break;
    }
    
    // Reset command buffer
    cmdLength = 0;
    cmdReady = false;
    responseIndex = 0;
    
    blinkLED(1);
}

// ============================================================================
// I2C Slave Callbacks
// ============================================================================

// Called when Pi sends data to hub
void onI2CReceive(int numBytes) {
    cmdLength = 0;
    while (WireBackbone.available() && cmdLength < sizeof(cmdBuffer)) {
        cmdBuffer[cmdLength++] = WireBackbone.read();
    }
    cmdReady = true;
}

// Called when Pi requests data from hub
void onI2CRequest() {
    if (responseIndex < responseLength) {
        WireBackbone.write(responseBuffer[responseIndex++]);
    } else {
        // Send error if overrun
        WireBackbone.write(ERROR_RESPONSE);
    }
}

// ============================================================================
// SERCOM Interrupt Handlers
// ============================================================================

// Required for WireBackbone (SERCOM5)
void SERCOM5_Handler(void) {
    WireBackbone.onService();
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
    // Initialize serial
    delay(1500);
    Serial.begin(115200);
    delay(1500);
    
    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    
    // Initialize I2C buses
    WireSensorA.begin();
    WireSensorB.begin();
    
    // Initialize backbone as I2C slave
    WireBackbone.begin(HUB_I2C_ADDRESS);
    WireBackbone.onReceive(onI2CReceive);
    WireBackbone.onRequest(onI2CRequest);
    
    // Configure pin peripheral states
    pinPeripheral(W0_SDA, PIO_SERCOM_ALT);
    pinPeripheral(W0_SCL, PIO_SERCOM_ALT);
    
    pinPeripheral(W1_SDA, PIO_SERCOM_ALT);
    pinPeripheral(W1_SCL, PIO_SERCOM_ALT);
    
    pinPeripheral(W2_SDA, PIO_SERCOM);
    pinPeripheral(W2_SCL, PIO_SERCOM);
    
    // Wait for serial
    while (!Serial);
    
    Serial.println("=====================================");
    Serial.println("  CureCraft SensorHub Firmware");
    Serial.println("=====================================");
    Serial.print("I2C Address: 0x");
    Serial.println(HUB_I2C_ADDRESS, HEX);
    Serial.println("Backbone: PA22 (SCL) / PA23 (SDA)");
    Serial.println("  -> Pi 400 GPIO 8/9 (I2C bus 3)");
    Serial.println("Sensors A: PA13 (SCL) / PA12 (SDA)");
    Serial.println("Sensors B: PA17 (SCL) / PA16 (SDA)");
    Serial.println("-------------------------------------");
    
    // Initial sensor scan
    scanAllSensors();
    
    Serial.println("Ready - waiting for Pi commands...");
    digitalWrite(LED_PIN, LOW);
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
    // Process any received commands
    if (cmdReady) {
        processCommand();
    }
    
    // Small delay to prevent busy-waiting
    delay(1);
}
