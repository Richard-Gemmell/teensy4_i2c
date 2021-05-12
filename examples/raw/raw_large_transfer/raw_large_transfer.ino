// Copyright © 2021 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// Large Transfers Loopback Test
// Demonstrates large transfers between a master and slave on the same Teensy.
//
// To use it, connect I2C ports 0 and 1 together.
//      Pin 16 <=> 19
//      Pin 17 <=> 18

#include <Arduino.h>
#include <i2c_driver.h>
#include "imx_rt1060/imx_rt1060_i2c_driver.h"

// Blink the LED to make sure the Teensy hasn't hung
IntervalTimer blink_timer;
volatile bool led_high = false;
void blink_isr();

// The Teensy 4 I2C hardware cannot be told to receive more than 256 bytes at once.
// Make sure the slave can supply more than this.
const size_t tx_buffer_size = UINT16_MAX + 512;
uint8_t tx_buffer[tx_buffer_size] = {};

// The master needs a suitably large buffer too
uint8_t rx_buffer[tx_buffer_size] = {};

const uint8_t slave_address = 0x2D;
I2CSlave& slave = Slave1;
void before_transmit_isr(uint16_t address);
void after_transmit_isr(uint16_t address);

I2CMaster& master = Master;

// Flags for handling callbacks
volatile bool before_transmit_received = false;
volatile bool after_transmit_received = false;
volatile I2CError slave_error = I2CError::ok;

void initialise_test_data();
void log_transmit_events();
void log_master_state(size_t bytes_requested);
void finish();

void setup() {
    // Blink the LED at regular intervals to prove we're alive
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, led_high);
    blink_timer.begin(blink_isr, 500'000);

    // Prepare the test data
    initialise_test_data();

    // Configure I2C Slave and start listening for messages
    slave.before_transmit(before_transmit_isr);
    slave.after_transmit(after_transmit_isr);
    slave.set_transmit_buffer(tx_buffer, tx_buffer_size);
    slave.listen(slave_address);

    // Configure I2C Master
    master.begin(1'000'000);

    // Enable the serial port for debugging
    Serial.begin(9600);
    Serial.println("Started large transfer test.");
}

void loop() {
    size_t bytes_requested = (256 * 257) + 10;
    Serial.print("Requesting ");Serial.print(bytes_requested);Serial.println(" bytes");

    master.read_async(slave_address, rx_buffer, bytes_requested, true);
    finish();
    log_transmit_events();
    log_master_state(bytes_requested);

//    Serial.println("End of Loop");
    Serial.println();
    delay(500);
}

// Called by an interrupt service routine.
// This function must be _very_ fast. Avoid IO.
void before_transmit_isr(uint16_t address) {
    before_transmit_received = true;
}

// Called by an interrupt service routine.
// This function must be _very_ fast. Avoid IO.
void after_transmit_isr(uint16_t address) {
    after_transmit_received = true;
    slave_error = slave.error();
}

void log_master_state(size_t bytes_requested) {
    if (master.has_error()) {
        Serial.print("Master reported error ");
        Serial.print((int)master.error());
        Serial.println();
    } else {
        Serial.print("Master received ");
        Serial.print(master.get_bytes_transferred());
        Serial.println(" bytes successfully.");
        Serial.print("Last byte is ");
        Serial.println(rx_buffer[bytes_requested - 1]);
    }
    memset(rx_buffer, 0, sizeof(rx_buffer));
}

void log_transmit_events() {
    if (before_transmit_received) {
        Serial.println("App: Transmission started.");
        before_transmit_received = false;
    }
    if (after_transmit_received) {
        Serial.println("App: Transmission complete.");
        after_transmit_received = false;
        if (slave_error != I2CError::ok) {
            Serial.print("App: Slave reports error ");
            Serial.print((int)slave_error);
            Serial.println();
        }
    }
}

void initialise_test_data() {
    uint8_t value = 0;
    for(size_t i=0; i<tx_buffer_size; i++) {
        if ((i % 256) == 0) {
            if (value == 0) {
                value = 0xFF;
            } else {
                value--;
            }
        }
        tx_buffer[i] = value;
    }
}

void finish() {
    elapsedMillis timeout;
    while (timeout < 5000) {
        if (master.finished()){
            Serial.print("Transfer complete in ");Serial.print((uint16_t)timeout);Serial.println(" ms");
            return;
        }
    }
    Serial.println("Master: ERROR timed out waiting for transfer to finish.");
}

void blink_isr() {
    led_high = !led_high;
    digitalWrite(LED_BUILTIN, led_high);
}
