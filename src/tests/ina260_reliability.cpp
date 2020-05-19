// Copyright © 2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// Tests the reliability of a connection by reading a sensor repeatedly.
// Requires an INA260 current sensor connected to pins 16 and 17.

//#define I2C_INA260_READ_RELIABILITY  // Uncomment to build this example
#ifdef I2C_INA260_READ_RELIABILITY

#include <Arduino.h>
#include <i2c_driver.h>
#include "imx_rt1060/imx_rt1060_i2c_driver.h"

// Blink the LED to make sure the Teensy hasn't hung
IntervalTimer blink_timer;
volatile bool led_high = false;
void blink_isr();

// The slave is an INA 260 current sensor
const uint16_t slave_address = 0x40;
const uint8_t manufacturer_id_register = 0xFE;
const uint8_t die_id_register = 0xFF;
I2CMaster& master = Master;

// Create a buffer to receive data from the slave.
uint8_t rx_buffer[2] = {};

uint32_t total_tests = 0;
uint32_t write_errors = 0;
uint32_t read_errors = 0;
uint32_t tests_per_batch = 50*1000;

void batch();
void report(uint32_t millis);
void read_and_check(uint8_t reg_num, uint16_t expected);
void reset_buffer();
uint16_t get_int_from_buffer();
bool finished_ok();

void setup() {
    // Turn the LED on
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, true);

    // Create a timer to blink the LED
    blink_timer.begin(blink_isr, 500000);

    // Initialise the master
    master.begin(1000 * 1001U);

    // Enable the serial port for debugging
    Serial.begin(9600);
    Serial.println("Started");
}

void loop() {
    batch();
}

void batch() {
    Serial.println("Starting batch");
    elapsedMillis time;
    uint32_t count = 0;
    while(count < tests_per_batch) {
        // Read the Manufacturer ID and check that it's correct
        read_and_check(manufacturer_id_register, 0x5449);
        // Request the Die ID
        read_and_check(die_id_register, 0x2270);
        count += 2;
    }
    report(time);
}

void log_line(const char* msg, uint32_t count, uint32_t total_count) {
    Serial.print(msg);
    Serial.print(count);
    Serial.print(" / ");
    Serial.print(total_count);
    Serial.print(" (");
    Serial.print((double)(100.0 * (float)count / (float)total_count), 4);
    Serial.println("%)");
}

void report(uint32_t millis) {
    log_line("  write errors    ", write_errors, total_tests);
    log_line("  read errors     ", read_errors, total_tests);
//    uint16_t bytes_per_test = sizeof(uint8_t) + sizeof(rx_buffer);
//    Serial.print("  ");
//    Serial.print(millis);
//    Serial.print("ms at ");
//    Serial.print((tests_per_batch * bytes_per_test) / (millis / 1000.0));
//    Serial.println(" bytes per second.");
}

void read_and_check(uint8_t reg_num, uint16_t expected) {
    total_tests++;
    reset_buffer();

    // Send reg number to read
    master.write_async(slave_address, (uint8_t*)&reg_num, sizeof(uint8_t), false);
    if (!finished_ok()) {
        write_errors++;
        return;
    }

    // Read data
    master.read_async(slave_address, rx_buffer, sizeof(rx_buffer), true);
    if (!finished_ok()) {
        read_errors++;
        return;
    }

    // Check data is correct
    uint16_t actual_value = get_int_from_buffer();
    if (actual_value != expected) {
        read_errors++;
    }
}

void reset_buffer() {
    memset(rx_buffer, 0, sizeof(rx_buffer));
}

uint16_t get_int_from_buffer() {
    uint16_t result = ((uint16_t)rx_buffer[0] << 8U) + ((uint16_t)rx_buffer[1]);
    return result;
}

bool finished_ok() {
    elapsedMillis timeout;
    while (timeout < 2000) {
        if (master.finished()){
            break;
        }
    }
    return master.finished() && !master.has_error();
}

void blink_isr() {
    led_high = !led_high;
    digitalWrite(LED_BUILTIN, led_high);
}

#endif