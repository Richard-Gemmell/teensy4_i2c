// Copyright © 2019-2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// This example WILL NOT work unless you have an INA260
// current sensor connected to pins 18 and 19.
//
// Demonstrates use of the I2C Device class to represent a slave device.
// Creates an I2C master, configures a device and reads registers.
//
// I recommend this approach for using the Teensy as an I2C slave.

#include <Arduino.h>
#include <i2c_device.h>

// The I2C device. Determines which pins we use on the Teensy.
I2CMaster& master = Master;

// Blink the LED to make sure the Teensy hasn't hung
IntervalTimer blink_timer;
volatile bool led_high = false;

// The slave is an INA 260 current sensor
const uint8_t slave_address = 0x40;
const uint8_t manufacturer_id_register = 0xFE;
const uint16_t expected_manufacturer_id = 0x5449;
const uint8_t die_id_register = 0xFF;
const uint16_t expected_die_id = 0x2270;
bool configured = false;
I2CDevice sensor = I2CDevice(master, slave_address, _BIG_ENDIAN);


bool configure_sensor();
void report_error(const char* message);

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    // Initialise the master
    master.begin(400 * 1000U);

    // Enable the serial port for debugging
    Serial.begin(9600);
    Serial.println("Started");

    // Check that we can see the sensor and configure it.
    configured = configure_sensor();
}

void loop() {
    if (configured) {
        int16_t current = 0;
        if (!sensor.read(0x01, &current, true)) {
            report_error("ERROR: Failed to read current");
        }
        Serial.printf("Current: %.0f mA\n", current * 1.25);

        int16_t voltage = 0;
        if (!sensor.read(0x02, &voltage, true)) {
            report_error("ERROR: Failed to read voltage");
        }
        Serial.printf("Voltage: %.0f mV\n", ((float)voltage) * 1.25);
    } else {
        Serial.println("Not configured");
    }

    // Blink the LED
    digitalWrite(LED_BUILTIN, HIGH);
    delay(900);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
}

bool configure_sensor() {
    // Check the manufacturer ID and check for I2C errors
    uint16_t manufacturerId;
    if (sensor.read(manufacturer_id_register, &manufacturerId, false)) {
        if (manufacturerId != expected_manufacturer_id) {
            Serial.printf("ERROR: Manufacturer ID is 0x%X. Expected 0x%X.\n", manufacturerId, expected_manufacturer_id);
            return false;
        }
    } else {
        report_error("ERROR: Failed to send manufacturer id register value");
        return false;
    }

    // Check the Die ID without checking for I2C errors
    uint16_t dieId = 0;
    sensor.read(die_id_register, &dieId, false);
    if (dieId != expected_die_id) {
        Serial.printf("ERROR: Die ID is 0x%X. Expected 0x%X.\n", dieId, expected_die_id);
        return false;
    }

    // Configure it
    uint16_t new_config = 0x6127U | 0x100U | 0x80U | 0x18U;
    if (!sensor.write(0x00, new_config, false)) {
        report_error("ERROR: Failed to set configuration register");
        return false;
    }
    if (!sensor.write(0x06, (uint16_t)0x0403, true)) {
        report_error("ERROR: Failed to set Mask/Enable Register");
        return false;
    }
    Serial.println("Configured sensor successfully.");
    return true;
}

void report_error(const char* message) {
    Serial.print(message);
    Serial.print(" Error: ");
    Serial.println((int)master.error());
}
