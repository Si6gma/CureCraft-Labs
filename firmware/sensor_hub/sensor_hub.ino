/*
 * SensorHub I2C Scanner - SAMD21
 *
 * Scans W1/W2 sensor buses and reports connected sensors to Raspberry Pi
 * Acts as I2C slave at 0x08 on backbone port
 */

#include <WireScanner.h>
#include "TwiPinHelper.h"

// I2C pin definitions
#define W0_SCL 27  // PA22 - Backbone (Pi connection)
#define W0_SDA 26  // PA23

#define W1_SCL 39  // PA13 - Sensor Bus A
#define W1_SDA 28  // PA12

#define W2_SCL 13  // PA17 - Sensor Bus B
#define W2_SDA 11  // PA16

// Pin pair helpers
TwiPinPair portBackbone(W0_SCL, W0_SDA);
TwiPinPair portSensorsA(W1_SCL, W1_SDA);
TwiPinPair portSensorsB(W2_SCL, W2_SDA);

// TwoWire instances
TwoWire WireBackbone(&sercom3, W0_SDA, W0_SCL);  // Pi connection (SERCOM3)
TwoWire WireSensorA(&sercom1, W1_SDA, W1_SCL);   // Sensor bus A (SERCOM1)
TwoWire WireSensorB(&sercom4, W2_SDA, W2_SCL);   // Sensor bus B (SERCOM4)

// I2C addresses
const uint8_t HUB_ADDRESS = 0x08;

// Sensor addresses (on W1/W2 buses)
const uint8_t ECG_ADDR  = 0x40;
const uint8_t SPO2_ADDR = 0x41;
const uint8_t TEMP_ADDR = 0x68;  // User's actual temp sensor address
const uint8_t NIBP_ADDR = 0x43;

// Protocol commands
const uint8_t CMD_PING = 0x00;
const uint8_t CMD_SCAN = 0x01;

// LED
#define LED_PIN 14

// State
volatile uint8_t lastCommand = 0;
volatile uint8_t sensorStatus = 0;
volatile bool needsScan = false;
volatile bool isScanning = false;  // Prevent scan interruptions

void setup() {
    delay(1500);
    Serial.begin(115200);
    delay(1500);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    // Initialize backbone as I2C slave
    WireBackbone.begin(HUB_ADDRESS);
    WireBackbone.onReceive(onReceive);
    WireBackbone.onRequest(onRequest);
    portBackbone.setPinPeripheralStates();  // Use setPinPeripheralStates() for SERCOM3

    // Initialize sensor buses as I2C masters
    WireSensorA.begin();
    portSensorsA.setPinPeripheralAltStates();  // Use AltStates for SERCOM1

    WireSensorB.begin();
    portSensorsB.setPinPeripheralStates();  // Use setPinPeripheralStates() for SERCOM4
    
    Serial.println("========================================");
    Serial.println("  SensorHub Scanner Firmware v1.0");
    Serial.println("========================================");
    Serial.print("Hub Address: 0x");
    Serial.println(HUB_ADDRESS, HEX);
    Serial.println("Backbone: PA22/PA23 (to Pi)");
    Serial.println("Sensor A: PA12/PA13 (W1)");
    Serial.println("Sensor B: PA16/PA17 (W2)");
    Serial.println("Ready!");
    Serial.println();

    // Do initial scan
    scanSensors();
    
    digitalWrite(LED_PIN, LOW);
}

void loop() {
    // ========================================================================
    // Auto-scan sensors every 5 seconds
    // ========================================================================
    static unsigned long lastScanTime = 0;
    const unsigned long SCAN_INTERVAL = 5000;  // 5 seconds
    
    if (millis() - lastScanTime >= SCAN_INTERVAL) {
        lastScanTime = millis();
        
        uint8_t previousStatus = sensorStatus;
        scanSensors();  // Updates sensorStatus
        
        // Detect and log changes
        if (sensorStatus != previousStatus) {
            Serial.println(">>> SENSOR STATUS CHANGED <<<");
            Serial.print("Previous: 0b");
            Serial.print(previousStatus, BIN);
            Serial.print(" -> New: 0b");
            Serial.println(sensorStatus, BIN);
            Serial.println();
        }
    }
    
    // ========================================================================
    // Blink LED slowly when idle
    // ========================================================================
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        lastBlink = millis();
    }

    // ========================================================================
    // Process manual scan requests (backward compatibility)
    // ========================================================================
    if (needsScan && !isScanning) {
        needsScan = false;
        scanSensors();
    }
}

void scanSensors() {
    isScanning = true;  // Prevent interruptions
    
    Serial.println("========================================");
    Serial.println("--- Auto-scanning Sensors ---");
    Serial.print("Time: ");
    Serial.print(millis() / 1000);
    Serial.println("s");
    Serial.println("========================================");
    
    sensorStatus = 0;
    
    // ========================================================================
    // Scan Bus A (W1) - Typically ECG, SpO2, and optionally Temperature
    // ========================================================================
    Serial.println("Bus A (W1):");
    
    // ECG sensor at 0x40
    if (probeSensor(WireSensorA, ECG_ADDR)) {
        sensorStatus |= (1 << 0);  // Bit 0 = ECG
        Serial.println("  ✓ ECG (0x40)");
    } else {
        Serial.println("  ✗ ECG");
    }
    
    // SpO2 sensor at 0x41
    if (probeSensor(WireSensorA, SPO2_ADDR)) {
        sensorStatus |= (1 << 1);  // Bit 1 = SpO2
        Serial.println("  ✓ SpO2 (0x41)");
    } else {
        Serial.println("  ✗ SpO2");
    }
    
    // Temperature sensor at 0x68 (W1 = Core Temp)
    if (probeSensor(WireSensorA, TEMP_ADDR)) {
        sensorStatus |= (1 << 2);  // Bit 2 = Core Temperature (W1)
        Serial.println("  ✓ Core Temp (0x68)");
    } else {
        Serial.println("  ✗ Core Temp");
    }
    
    // ========================================================================
    // Scan Bus B (W2) - Typically NIBP and optionally Temperature
    // ========================================================================
    Serial.println("Bus B (W2):");
    Serial.flush();  // Ensure message is sent before potential hang
    
    // NIBP sensor at 0x43
    Serial.print("  Probing NIBP...");
    Serial.flush();
    if (probeSensor(WireSensorB, NIBP_ADDR)) {
        sensorStatus |= (1 << 3);  // Bit 3 = NIBP
        Serial.println(" ✓ (0x43)");
    } else {
        Serial.println(" ✗");
    }
    
    // Temperature sensor on W2 (W2 = Skin Temp) - Independent check
    Serial.print("  Probing Skin Temp...");
    Serial.flush();
    if (probeSensor(WireSensorB, TEMP_ADDR)) {
        sensorStatus |= (1 << 4);  // Bit 4 = Skin Temperature (W2)
        Serial.println(" ✓ (0x68)");
    } else {
        Serial.println(" ✗");
    }
    
    Serial.println("========================================");
    Serial.print("Status byte: 0b");
    Serial.print(sensorStatus, BIN);
    Serial.print(" (0x");
    Serial.print(sensorStatus, HEX);
    Serial.println(")");
    Serial.println("========================================");
    Serial.println();
    
    isScanning = false;  // Scan complete
}

bool probeSensor(TwoWire &wire, uint8_t addr) {
    wire.beginTransmission(addr);
    uint8_t error = wire.endTransmission();
    return (error == 0);  // 0 means ACK received
}

// Called when Pi sends data
void onReceive(int numBytes) {
    if (numBytes > 0) {
        lastCommand = WireBackbone.read();
        
        Serial.print("Received command: 0x");
        Serial.println(lastCommand, HEX);
        
        if (lastCommand == CMD_SCAN) {
            needsScan = true;  // Scan in main loop
        }
        
        // Consume any extra bytes
        while (WireBackbone.available()) {
            WireBackbone.read();
        }
    }
}

// Called when Pi requests data
void onRequest() {
    if (lastCommand == CMD_PING) {
        // Respond with 0x42 (magic ping response)
        WireBackbone.write(0x42);
        Serial.println("Sent: PING response (0x42)");
    }
    else if (lastCommand == CMD_SCAN) {
        // Send sensor status byte
        WireBackbone.write(sensorStatus);
        Serial.print("Sent: Status 0b");
        Serial.println(sensorStatus, BIN);
    }
    else {
        // Unknown command - send 0x00
        WireBackbone.write(0x00);
        Serial.println("Sent: 0x00 (unknown command)");
    }
}
