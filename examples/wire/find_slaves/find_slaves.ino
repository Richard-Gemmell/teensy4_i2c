#include <Arduino.h>
#include <i2c_driver_wire.h>

void trace(const char* message);
void trace(const char* message, uint8_t address);

void setup() {
    Wire.begin();

    Serial.begin(9600);
    while (!Serial);
}

bool enable_trace = false; // Set to true for more logging
void loop() {
    Serial.println("Searching for slave devices...");

    uint8_t num_found = 0;
    for (uint8_t address = 1; address < 127; address++) {
        trace("Checking address ", address);
        Wire.beginTransmission(address);

        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            Serial.print("Slave device found at address ");
            Serial.println(address);
            num_found++;
        } else if (error == 2) {
            // The master received an address NACK.
            // This is the code we expect if there's nobody listening on this address.
            trace("Address not in use.");
        } else {
            Serial.print("Unexpected error at address ");
            Serial.println(address);
        }
        trace("");
    }
    if (num_found == 0) {
        Serial.println("No I2C slave devices found.");
    } else {
        Serial.print("Found ");
        Serial.print(num_found);
        Serial.println(" slave devices.");
    }
    Serial.println();

    delay(5000);           // wait 5 seconds for next scan
}

void trace(const char* message) {
    if (enable_trace) {
        Serial.println(message);
    }
}

void trace(const char* message, uint8_t address) {
    if (enable_trace) {
        Serial.print(message);
        Serial.println(address);
    }
}
