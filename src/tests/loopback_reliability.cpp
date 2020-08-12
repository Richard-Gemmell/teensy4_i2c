// Copyright © 2019-2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

//#define I2C_RAW_LOOPBACK_RELIABILITY  // Uncomment to build this example
#ifdef I2C_RAW_LOOPBACK_RELIABILITY

#include <Arduino.h>
#include "i2c_driver.h"
#include "imx_rt1060/imx_rt1060_i2c_driver.h"

// Blink the LED to make sure the Teensy hasn't hung
IntervalTimer blink_timer;
volatile bool led_high = false;
void blink_isr();

const bool stop = true;
const bool no_stop = false;

// Master
I2CMaster& master = Master;

// Slave
const uint16_t slave_address = 0x002D;
I2CSlave& slave = Slave1;
void after_slave_receive(size_t length, uint16_t address);

// Data to exchange
uint8_t master_tx_buffer[] = {0xAA};

// Slave data
const size_t slave_rx_buffer_size = 2;
uint8_t slave_rx_buffer[slave_rx_buffer_size] = {};
volatile size_t slave_bytes_received = 0;
uint8_t slave_tx_buffer[] = {0xC3};
void after_transmit(uint16_t address);
volatile bool after_transmit_received = false;
volatile bool buffer_underflow_detected = false;

uint32_t master_write_success_count = 0;
uint32_t master_write_fail_count = 0;
uint32_t master_read_success_count = 0;
uint32_t master_read_fail_count = 0;
uint32_t master_good_read_data_count = 0;
uint32_t master_bad_read_data_count = 0;
uint32_t master_timeout_count = 0;

uint32_t slave_received_count = 0;
uint32_t slave_transmitted_count = 0;
uint32_t slave_read_error_count = 0;
uint32_t slave_write_error_count = 0;
uint32_t slave_good_read_data_count = 0;
uint32_t slave_bad_read_data_count = 0;


void log_line(const char* msg, uint32_t count, uint32_t other_count) {
    uint32_t total_count = count + other_count;
    Serial.print(msg);
    Serial.print(count);
    Serial.print(" / ");
    Serial.print(total_count);
    Serial.print(" (");
    Serial.print((double)(100.0 * (float)count / (float)total_count), 4);
    Serial.println("%)");
}

void log_failure_count() {
    Serial.println("Master:");
    log_line("  writes succeeded ", master_write_success_count, master_write_fail_count);
    log_line("  writes failed    ", master_write_fail_count, master_write_success_count);
    log_line("  reads suceeded   ", master_read_success_count, master_read_fail_count);
    log_line("  reads failed     ", master_read_fail_count, master_read_success_count);
    log_line("  bad read bytes   ", master_read_fail_count, master_good_read_data_count);
    Serial.print("  timeouts:        ");
    Serial.println(master_timeout_count);

    Serial.println("Slave:");
    log_line("  failed writes  ", slave_write_error_count, slave_transmitted_count - slave_write_error_count);
    log_line("  failed reads   ", slave_read_error_count, slave_received_count - slave_read_error_count);
    log_line("  bad read bytes ", slave_bad_read_data_count, slave_good_read_data_count);
    Serial.println();
}

void setup() {
    // Turn the LED on
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, true);
    // Create a timer to blink the LED
    blink_timer.begin(blink_isr, 500000);

    // Configure I2C Slave
    slave.after_receive(after_slave_receive);
    slave.set_receive_buffer(slave_rx_buffer, slave_rx_buffer_size);
    slave.after_transmit(after_transmit);
    slave.set_transmit_buffer(slave_tx_buffer, sizeof(slave_tx_buffer));
    slave.listen(slave_address);

    // Initialise the master
    master.begin(1000 * 1000U);

    // Enable the serial port for debugging
    Serial.begin(9600);
    Serial.println("App Started");
}


void master_write_then_read();
void finish();

uint32_t last_update = 0;

void loop() {
    master_write_then_read();

    uint32_t now = millis();
    if (now - last_update > 5000) {
        last_update = now;
        log_failure_count();
    }
//    delay(10);
}

void master_write_then_read() {
    master.write_async(slave_address, master_tx_buffer, sizeof(master_tx_buffer), stop);
    finish();

    if (master.has_error()) {
//        Serial.print("FAIL: App Master: Failed to write register. Error: ");
//        Serial.println((int) master.error());
        master_write_fail_count++;
//        log_failure_count();
    }
    else {
        master_write_success_count++;

        uint8_t read_buffer[1] = {};
        master.read_async(slave_address, read_buffer, sizeof(read_buffer), stop);
        finish();
        if (read_buffer[0] != 0xC3) {
            master_bad_read_data_count++;
        } else {
            master_good_read_data_count++;
        }

        if (master.has_error()) {
            master_read_fail_count++;
//            Serial.print("FAIL: App Master: Failed to read register. Error: ");
//            Serial.println((int) master.error());
//            log_failure_count();
        } else {
            master_read_success_count++;
        }
    }
}

void after_slave_receive(size_t length, uint16_t address) {
    if (length != 1) {
        Serial.print("Freaky message size: ");Serial.println(length);
    }
    slave_received_count++;
    if (slave.has_error()) {
        slave_read_error_count++;
    }
    if (slave_rx_buffer[0] != 0xAA) {
        slave_bad_read_data_count++;
    } else {
        slave_good_read_data_count++;
    }
    memset(slave_rx_buffer, 0, sizeof(slave_rx_buffer));
}

void after_transmit(uint16_t address) {
    slave_transmitted_count++;
    if (slave.has_error()) {
        slave_write_error_count++;
    }
}

void finish() {
    elapsedMillis timeout;
    while (timeout < 200) {
        if (master.finished()){
            return;
        }
    }
    master_timeout_count++;
    Slave1.reset();
//    Serial.println("Master: ERROR timed out waiting for transfer to finish.");
}

void blink_isr() {
    led_high = !led_high;
    digitalWrite(LED_BUILTIN, led_high);
}

#endif