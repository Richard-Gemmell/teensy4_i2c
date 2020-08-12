// Copyright © 2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// Tests the reliability of a connection by exchanging known data
// with a Raspberry Pi I2C master.
// Assumes the master is connected to pins 16 and 17.

//#define I2C_RASPBERRY_PI_RELIABILITY  // Uncomment to build this example
#ifdef I2C_RASPBERRY_PI_RELIABILITY

#include <Arduino.h>
#include <i2c_driver.h>
#include "imx_rt1060/imx_rt1060_i2c_driver.h"

// Blink the LED to make sure the Teensy hasn't hung
IntervalTimer blink_timer;
volatile bool led_high = false;
void blink_isr();

const uint16_t slave_address = 0x2D;
I2CSlave& slave = Slave1;

// Create a buffer to receive data from the master
uint8_t rx_buffer[40] = {};
uint8_t message[] = "vast and cool and unsympathetic";
uint8_t received_message_ok = 0;

void reset_rx_buffer();
void report();
void after_transmit(uint16_t address);
void after_receive(size_t length, uint16_t address);

uint32_t read_count = 0;
uint32_t read_errors = 0;
uint32_t bytes_read = 0;
uint32_t write_count = 0;
uint32_t write_errors = 0;

void setup() {
    // Enable the serial port for debugging
    Serial.begin(9600);
    Serial.println("Started");

    // Turn the LED on
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, true);

    // Create a timer to blink the LED
    blink_timer.begin(blink_isr, 500000);

    // Initialise the master
    slave.listen(slave_address);
    slave.set_receive_buffer(rx_buffer, sizeof(rx_buffer));
    slave.after_receive(after_receive);
    slave.after_transmit(after_transmit);
}

void loop() {
    Serial.println("Loop");
    delay(1000);
}

void after_transmit(uint16_t address) {
    write_count++;
    if (slave.has_error()) {
        write_errors++;
        report();
    }
}

void after_receive(size_t length, uint16_t address) {
    if (slave.has_error()) {
        read_errors += 1;
    }
    bytes_read += length;
    if (length == 1) {
        // Master is just setting the register number
        uint8_t reg_num = rx_buffer[0];
//        Serial.print("Requesting register ");
//        Serial.println(reg_num);
        if (reg_num == 0) {
            // Master wants to read the string message
            slave.set_transmit_buffer(message, sizeof(message));
        } else if (reg_num == 1) {
            // Master wants to know if the previous write succeeded
            slave.set_transmit_buffer(&received_message_ok, sizeof(received_message_ok));
        } else if (reg_num == 10) {
            // Master is about to write a message
            // Note that this never happens in practice. It's just here for completeness.
            received_message_ok = 0;
        }
    } else {
        // Master has written a message. Check that it's Ok.
        received_message_ok = 0;
        read_count += 1;
        // WARNING: Note that the first byte is the register number. Ignore it.
        char diff = strncmp((const char*)rx_buffer+1, (const char*)message, sizeof(message));
        if (diff != 0) {
            read_errors += 1;
//            Serial.println((const char*)rx_buffer);
        } else {
//            Serial.println("Message received OK");
            received_message_ok = 0xAA;
        }
    }
    reset_rx_buffer();
}

void report() {
    Serial.print("Message sent. ");
    Serial.print(write_errors);
    Serial.print(" errors from ");
    Serial.print(write_count);
    Serial.println(" messages transmitted.");
}

void reset_rx_buffer() {
    memset(rx_buffer, 0, sizeof(rx_buffer));
}

void blink_isr() {
    led_high = !led_high;
    digitalWrite(LED_BUILTIN, led_high);
}

#endif