/*
 * SensorHub I2C Scanner - SAMD21
 *
 * Scans W1/W2 sensor buses and reports connected sensors to Raspberry Pi
 * Acts as I2C slave at 0x08 on backbone port
 */

#include <Wire.h>
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
TwoWire WireBackbone(&sercom5, W0_SDA, W0_SCL);  // Pi connection
TwoWire WireSensorA(&sercom1, W1_SDA, W1_SCL);   // Sensor bus A
TwoWire WireSensorB(&sercom4, W2_SDA, W2_SCL);   // Sensor bus B

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
    portBackbone.setPinPeripheralAltStates();

    // Initialize sensor buses as I2C masters
    WireSensorA.begin();
    pinPeripheral(W1_SDA, PIO_SERCOM);
    pinPeripheral(W1_SCL, PIO_SERCOM);

    WireSensorB.begin();
    pinPeripheral(W2_SDA, PIO_SERCOM);
    pinPeripheral(W2_SCL, PIO_SERCOM);

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
    // Blink LED slowly when idle
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        lastBlink = millis();
    }

    // Process scan requests (only if not already scanning)
    if (needsScan && !isScanning) {
        needsScan = false;
        scanSensors();
    }
}

void scanSensors() {
    isScanning = true;  // Prevent interruptions
    
    Serial.println("--- Scanning Sensors ---");
    
    sensorStatus = 0;
    
    // Scan Bus A (W1) for temp sensor
    Serial.println("Bus A (W1):");
    if (probeSensor(WireSensorA, TEMP_ADDR)) {
        sensorStatus |= (1 << 0);  // Bit 0 = Temp sensor on W1
        Serial.println("  ✓ Core Temp (0x68)");
    } else {
        Serial.println("  ✗ Core Temp");
    }
    
    // Scan Bus B (W2) for temp sensor
    Serial.println("Bus B (W2):");
    if (probeSensor(WireSensorB, TEMP_ADDR)) {
        sensorStatus |= (1 << 1);  // Bit 1 = Temp sensor on W2
        Serial.println("  ✓ Skin Temp (0x68)");
    } else {
        Serial.println("  ✗ Skin Temp");
    }
    
    Serial.print("Status byte: 0b");
    Serial.println(sensorStatus, BIN);
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

// SERCOM5 interrupt handler
void SERCOM5_Handler(void) {
    WireBackbone.onService();
}
