// Copyright © 2019 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)
//
// Demonstrates use of the raw I2C driver.
// Creates an I2C master and probes the bus to find any listening slave devices.

#include <Arduino.h>
#include <i2c_driver.h>
#include "imx_rt1060/imx_rt1060_i2c_driver.h"

I2CMaster& master = Master;
void finish(bool report_timeout);
void trace(const char* message);
void trace(const char* message, uint8_t address);

void setup() {
    // Initialise the master
    master.begin(100 * 1000U);

    Serial.begin(9600);
    while (!Serial);
}

bool enable_trace = false; // Set to true for more logging
void loop() {
    Serial.println("Searching for slave devices...");

    uint8_t num_found = 0;
    uint8_t buffer[] = {0};
    for (uint8_t address = 1; address < 127; address++) {
        trace("Checking address ", address);
        master.read_async(address, buffer, 1, true);
        finish(false);
        master.write_async(address, buffer, 0, true);
        finish(true);

        I2CError status = master.error();
        if (status == I2CError::ok) {
            Serial.print("Slave device found at address ");
            Serial.println(address);
            num_found++;
        } else if (status == I2CError::address_nak) {
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

    delay(3'000);   // wait a while before scanning again
}

void finish(bool report_timeout) {
    elapsedMillis timeout;
    while (timeout < 200) {
        if (master.finished()){
            return;
        }
    }
    if (report_timeout) {
        Serial.println("Master: ERROR timed out waiting for transfer to finish.");
    }
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
