#include <Wire.h>

// Definitions from i2c_protocol.h
#define HUB_I2C_ADDRESS 0x08
#define CMD_PING 0x00
#define CMD_SCAN 0x01
#define PING_RESPONSE 0x42
#define MOCK_STATUS 0x1F // All sensors attached

volatile uint8_t lastCommand = 0xFF;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting I2C Slave Test...");

  Wire.begin(HUB_I2C_ADDRESS);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  Serial.print("I2C Slave Listening on address 0x");
  Serial.println(HUB_I2C_ADDRESS, HEX);
}

void loop() {
  delay(100);
}

// Called when Pi sends data (writes)
void receiveEvent(int howMany) {
  if (howMany > 0) {
    lastCommand = Wire.read();
    Serial.print("Rx Cmd: 0x");
    Serial.println(lastCommand, HEX);
    
    // Flush remaining
    while (Wire.available()) {
      Wire.read();
    }
  }
}

// Called when Pi requests data (reads)
void requestEvent() {
  Serial.print("Tx Req for Cmd: 0x");
  Serial.println(lastCommand, HEX);

  if (lastCommand == CMD_PING) {
    Wire.write(PING_RESPONSE); // 0x42
  } else if (lastCommand == CMD_SCAN) {
    Wire.write(MOCK_STATUS); // 0x1F
  } else {
    Wire.write(0xEE); // Unknown
  }
}
