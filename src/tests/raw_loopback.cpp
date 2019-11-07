// Copyright © 2019 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// Demonstrates use of the raw I2C driver as a slave receiver.
// Receives data from a master device.

//#define I2C_RAW_LOOPBACK_EXAMPLE  // Uncomment to build this example
#ifdef I2C_RAW_LOOPBACK_EXAMPLE

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
void log_master_transferred(uint8_t* buffer, size_t num_bytes);

// Slave
const uint16_t slave_address = 0x002D;
I2CSlave& slave = Slave1;
void after_receive(int size);

// Double receive buffers to hold data from master.
const size_t slave_rx_buffer_size = 5;
uint8_t slave_rx_buffer[slave_rx_buffer_size] = {};
uint8_t slave_rx_buffer_2[slave_rx_buffer_size] = {};
volatile size_t slave_bytes_received = 0;
void log_slave_message_received();

// Set up slave transmissions
const size_t slave_tx_buffer_size = 6;
uint8_t slave_tx_buffer[slave_tx_buffer_size] = {0x14, 0x15, 0x16, 0x17, 0x18, 0x19};
void after_transmit();
volatile bool after_transmit_received = false;
volatile bool buffer_underflow_detected = false;
void log_transmit_events();

void setup() {
    // Turn the LED on
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, true);
    // Create a timer to blink the LED
    blink_timer.begin(blink_isr, 500000);

    // Configure I2C Slave
    slave.after_receive(after_receive);
    slave.set_receive_buffer(slave_rx_buffer, slave_rx_buffer_size);
    slave.after_transmit(after_transmit);
    slave.set_transmit_buffer(slave_tx_buffer, slave_tx_buffer_size);
    slave.listen(slave_address);

    // Initialise the master
    master.begin(1000 * 1000U);

    // Enable the serial port for debugging
    Serial.begin(9600);
    Serial.println("App Started");
}

uint32_t success_count = 0;
uint32_t fail_count = 0;

void log_failure_count() {
    Serial.print("App Master: ");
    Serial.print(fail_count);
    Serial.print(" failed. ");
    Serial.print(success_count);
    Serial.println(" succeeded. ");
}

void finish();
void test_master_read();
void test_master_sequential_reads();
void test_master_invalid_slave_address();
void test_slave_read_buffer_overrun();
void test_master_read_too_large();
void test_master_write();
void test_write_then_read();

bool done = false;
void loop() {
    // Test cases
    if(!done) {
        test_master_read();                     // Happy case reading bytes from the slave
//        test_master_invalid_slave_address();    // Got the slave address wrong
//        test_master_sequential_reads();         // Show that the user can hold the bus before doing another operation.
//        test_slave_read_buffer_overrun();       // Slave should NACK if asked for too many bytes
//        test_master_read_too_large();           // Master cannot read this many bytes in one go
//        test_master_write();                    // Happy case sending bytes to the slave
//        test_write_then_read();                 // The standard pattern of requesting a register value
//        done = true;
    }
//    done = true;

    // Report slave transactions
    log_transmit_events();
    if (slave_bytes_received) {
        log_slave_message_received();
        slave_bytes_received = 0;
    }

//    led_high = !led_high;
//    digitalWrite(LED_BUILTIN, led_high);
    delay(1000);
}

void finish() {
    elapsedMillis timeout;
    while (timeout < 200) {
        if (master.finished()){
            return;
        }
    }
    Serial.println("Master: ERROR timed out waiting for transfer to finish");
}

void test_master_read() {
    Serial.println();
    uint8_t read_buffer[6] = {};
    master.read_async(slave_address, read_buffer, sizeof(read_buffer), stop);
    finish();

    if (!master.has_error()) {
        log_master_transferred(read_buffer, sizeof(read_buffer));
        success_count++;
        log_failure_count();
    } else {
        Serial.print("FAIL: App Master: Failed to read. Error: ");
        Serial.println((int)master.error());
        fail_count++;
        log_failure_count();
    }
}

void test_master_sequential_reads() {
    // Show that the user can hold the bus before doing another operation.
    uint8_t read_buffer[6] = {};
    master.read_async(slave_address, read_buffer, sizeof(read_buffer), no_stop);
    finish();

    uint8_t read_buffer_2[2] = {};
    master.read_async(slave_address, read_buffer_2, sizeof(read_buffer_2), stop);
    finish();

    if (!master.has_error()) {
        log_master_transferred(read_buffer, sizeof(read_buffer));
        log_master_transferred(read_buffer_2, sizeof(read_buffer_2));
        success_count++;
        log_failure_count();
    } else {
        Serial.print("FAIL: App Master: Failed to read. Error: ");
        Serial.println((int)master.error());
        fail_count++;
        log_failure_count();
    }
}

void test_master_invalid_slave_address() {
    // Use in invalid address to force an address NACK
    uint8_t read_buffer[6] = {};
    master.read_async(0x11, read_buffer, sizeof(read_buffer), stop);
    finish();
    if (master.has_error()) {
        if (master.error() == I2CError::address_nak) {
            Serial.println("PASS: App Master: Attempted to send to invalid address.");
        } else {
            Serial.print("FAIL: App Master: Attempted to send to invalid address. Expected address_nak but got: ");
            Serial.println((int)master.error());
        }
    } else {
        Serial.println("FAIL: App Master: ERROR Unexpected success reading from invalid address.");
    }
}

void test_master_read_too_large() {
    uint8_t read_buffer[257] = {};
    master.read_async(slave_address, read_buffer, sizeof(read_buffer), stop);
    finish();
    if (master.has_error()) {
        if (master.error() != I2CError::invalid_request) {
            Serial.print("FAIL: App Master: Unexpected error attempting to read too many bytes. Expected invalid_request. Got : ");
            Serial.println((int)master.error());
        } else {
            Serial.println("PASS: App Master: Attempted to read too many bytes.");
        }
    } else {
        Serial.print("FAIL: App Master: Unexpected success readying too many bytes. Error: ");
        Serial.println((int) master.error());
    }
}

// WARNING: Doesn't work at the moment because the Slave doesn't NACK
// it just sends dummy data instead.
void test_slave_read_buffer_overrun() {
    // Ask for too many bytes to force a data NACK
    uint8_t read_buffer[slave_tx_buffer_size+2] = {};
    master.read_async(slave_address, read_buffer, sizeof(read_buffer), stop);
    finish();
    if (master.has_error()) {
        Serial.print("PASS: App Master: Attempted to read too many bytes. Error: ");
        Serial.println((int) master.error());
    } else {
        Serial.println("FAIL: App Master: Unexpected success reading too many bytes.");
    }
    if (buffer_underflow_detected) {
        Serial.println("PASS: App Slave: Buffer underflow reported correctly.");
    } else {
        Serial.println("FAIL: App Slave: Should have reported buffer underflow.");
    }
}

void log_master_transferred(uint8_t* buffer, size_t num_bytes) {
    Serial.print("App Master: transferred ");
    for (size_t i = 0; i < num_bytes; i++) {
        Serial.print(buffer[i]);
        if(i< num_bytes-1) {
            Serial.print(", ");
        }
    }
    Serial.println(".");
}

void test_master_write() {
    Serial.println();
    uint8_t write_buffer[5] = {0x40, 0x41, 0x42, 0x43, 0x44};
    master.write_async(slave_address, write_buffer, sizeof(write_buffer), stop);
    finish();

    if (!master.has_error()) {
        log_master_transferred(write_buffer, sizeof(write_buffer));
        success_count++;
        log_failure_count();
    } else {
        Serial.print("FAIL: App Master: Failed to write. Error: ");
        Serial.println((int) master.error());
        fail_count++;
        log_failure_count();
    }
}

void test_write_then_read() {
    Serial.println();
    uint8_t write_buffer[1] = {0x40};
    master.write_async(slave_address, write_buffer, sizeof(write_buffer), no_stop);
    finish();

    if (master.has_error()) {
        Serial.print("FAIL: App Master: Failed to write register. Error: ");
        Serial.println((int) master.error());
        fail_count++;
        log_failure_count();
    }
    else {
        uint8_t read_buffer[2] = {};
        master.read_async(slave_address, read_buffer, sizeof(read_buffer), stop);
        finish();

        if (!master.has_error()) {
            log_master_transferred(read_buffer, sizeof(read_buffer));
            success_count++;
            log_failure_count();
        } else {
            Serial.print("FAIL: App Master: Failed to read from register. Error: ");
            Serial.println((int) master.error());
            fail_count++;
            log_failure_count();
        }
    }
}

void after_receive(int size) {
    if (!slave_bytes_received) {
        memcpy(slave_rx_buffer_2, slave_rx_buffer, size);
        slave_bytes_received = size;
    }
}

void log_slave_message_received() {
    if (slave.error() == I2CError::buffer_overflow) {
        Serial.println("App Slave: Buffer Overflow. (Master sent too many bytes.)");
    }
    Serial.print("App Slave: Slave received ");
    Serial.print(slave_bytes_received);
    Serial.print(" bytes: ");
    for(size_t i=0; i < slave_bytes_received; i++) {
        Serial.print(slave_rx_buffer_2[i]);
        Serial.print(", ");
    }
    Serial.println();
}

void after_transmit() {
    after_transmit_received = true;
    if (slave.error() == I2CError::buffer_underflow) {
        buffer_underflow_detected = true;
    }
    // Get ready for the next request
    slave.set_transmit_buffer(slave_tx_buffer, slave_tx_buffer_size);
}

void log_transmit_events() {
    if (after_transmit_received) {
//        Serial.println("App Slave: Transmission complete.");
        after_transmit_received = false;
        if (buffer_underflow_detected) {
            Serial.println("App Slave: Buffer Underflow. (Master asked for too many bytes.)");
            buffer_underflow_detected = false;
        }
    }
}

void blink_isr() {
    led_high = !led_high;
    digitalWrite(LED_BUILTIN, led_high);
}

#endif