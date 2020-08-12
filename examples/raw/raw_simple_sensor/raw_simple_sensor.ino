// Copyright © 2019-2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// A Simple Sensor
// Demonstrates use of the raw I2C driver as a simple slave transmitter.
// The sensor provides the master with a simulated temperature reading
// on demand. The temperature increases each time the master reads it.
//
// To use it, connect a master to the Teensy on pins 16 and 17.
// Send read requests to the Teensy.
//
// This is an advanced example. Use the "simple" examples
// instead if you want to follow the typical usage pattern
// for I2C.

#include <Arduino.h>
#include <i2c_driver.h>
#include "imx_rt1060/imx_rt1060_i2c_driver.h"

// Blink the LED to make sure the Teensy hasn't hung
IntervalTimer blink_timer;
volatile bool led_high = false;
void blink_isr();

const uint8_t slave_address = 0x2D;
I2CSlave& slave = Slave1;
void set_temp(uint16_t new_temp);
void before_transmit_isr(uint16_t address);
void after_transmit(uint16_t address);

// Set up transmit buffer and temperature value
const size_t tx_buffer_size = 2;
uint8_t tx_buffer[tx_buffer_size] = {};
uint16_t fake_temp = 1;

// Flags for handling callbacks
volatile bool before_transmit_received = false;
volatile bool after_transmit_received = false;
volatile bool buffer_underflow_detected = false;
void log_transmit_events();


void setup() {
    // Turn the LED on
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, true);

    // Create a timer to blink the LED
    blink_timer.begin(blink_isr, 500 * 1000);

    // Initialise the data we'll send to the master
    set_temp(fake_temp);

    // Configure I2C Slave and Start It
    slave.before_transmit(before_transmit_isr);
    slave.after_transmit(after_transmit);
    slave.set_transmit_buffer(tx_buffer, tx_buffer_size);
    slave.listen(slave_address);

    // Enable the serial port for debugging
    Serial.begin(9600);
    Serial.println("Started");
}

void loop() {
    // Now we've got the time, we can handle the results of the ISR callbacks.
    log_transmit_events();

    // The master can read the sensor as often as it likes while we sleep
    delay(2);
}

// Sets the value that will be returned to the master
void set_temp(uint16_t new_temp) {
    memcpy(tx_buffer, &new_temp, sizeof(new_temp));
    Serial.print("Setting temperature to ");
    Serial.println(new_temp);
}

// Called by an interrupt service routine.
// This function must be _very_ fast. Avoid IO.
void before_transmit_isr(uint16_t address) {
    before_transmit_received = true;
}

// Called by an interrupt service routine.
// This function must be _very_ fast. Avoid IO.
void after_transmit(uint16_t address) {
    after_transmit_received = true;
    if (slave.has_error()) {
        I2CError error = slave.error();
        if (error == I2CError::buffer_underflow) {
            buffer_underflow_detected = true;
        } else {
            Serial.println("Unexpected error");
        }
    }
}

void log_transmit_events() {
    if (before_transmit_received) {
        Serial.println("App: Transmission started.");
        before_transmit_received = false;
    }
    if (after_transmit_received) {
        Serial.println("App: Transmission complete.");
        after_transmit_received = false;
        if (buffer_underflow_detected) {
            Serial.println("App: Buffer Underflow. (Master asked for too many bytes.)");
            buffer_underflow_detected = false;
        }
        Serial.println();

        // Use a new temperature
        set_temp(++fake_temp);
    }
}

void blink_isr() {
    led_high = !led_high;
    digitalWrite(LED_BUILTIN, led_high);
}
