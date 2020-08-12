// Copyright © 2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// Shows how an I2C slave can listen to more than one address.
// This is handy if you want the Teensy to impersonate a lot of different devices.
//
// To use it, connect a master to the Teensy on pins 16 and 17.
// Use the master to query the Teensy on different addresses.
// The Teensy echoes the I2C address back to the master.
//
// This is an advanced example.

#include <Arduino.h>
#include <i2c_driver.h>
#include "imx_rt1060/imx_rt1060_i2c_driver.h"

// Blink the LED to make sure the Teensy hasn't hung
IntervalTimer blink_timer;
volatile bool led_high = false;
void blink_isr();

I2CSlave& slave = Slave1;
void after_receive(size_t length, uint16_t address);
void before_transmit(uint16_t address);
void after_transmit(uint16_t address);

// Buffers to hold data received from master.
const size_t rx_buffer_size = sizeof(uint8_t);
uint8_t rx_buffer[rx_buffer_size] = {0};
volatile uint8_t receive_address = 0;
volatile size_t slave_bytes_received = 0;

// We're going to echo the requested address back to the master
// This buffer holds the address.
const size_t tx_buffer_size = sizeof(uint8_t);
uint8_t tx_buffer[tx_buffer_size] = {0};
volatile bool transmitted = false;
volatile uint8_t before_transmit_address = 0;
volatile uint8_t after_transmit_address = 0;

void log_received_address();
void log_transmitted_address();

void setup() {
  // Turn the LED on
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, true);

  // Create a timer to blink the LED
  blink_timer.begin(blink_isr, 500000);

  // Configure I2C Slave
  slave.set_receive_buffer(rx_buffer, rx_buffer_size);
  slave.set_transmit_buffer(tx_buffer, tx_buffer_size);
  slave.after_receive(after_receive);
  slave.before_transmit(before_transmit);
  slave.after_transmit(after_transmit);

  // Listen to every addresses in the range
  slave.listen_range(0x10, 0x20);

  // Use this call to listen() if you want to listen to 2 addresses
  // without listening to every address in the range.
//  slave.listen(0x10, 0x20);

  // Enable the serial port for debugging
  Serial.begin(9600);
  Serial.println("Started");
}

void loop() {
    if (slave_bytes_received) {
        log_received_address();
    }
    if (transmitted) {
        log_transmitted_address();
    }

    // We could receive multiple message while we're asleep.
    // This example is modelling an application where it's Ok
    // to drop messages.
    delay(2);
}

// Called by the I2C interrupt service routine.
// This method must be as fast as possible.
// Do not perform IO in it.
void after_receive(size_t length, uint16_t address) {
    receive_address = address;
    slave_bytes_received = length;
}

void before_transmit(uint16_t address) {
    before_transmit_address = address;
    uint8_t byte_address = before_transmit_address;
    memcpy(tx_buffer, &byte_address, tx_buffer_size);
}

void after_transmit(uint16_t address) {
    after_transmit_address = address;
    transmitted = true;
}

void log_received_address() {
    if (slave.has_error()) {
        Serial.println("App: Unexpected error");
    }
    Serial.print("App: Received ");
    Serial.print(slave_bytes_received);
    Serial.print(" bytes on address 0x");
    Serial.print(receive_address, HEX);
    Serial.println();

    slave_bytes_received = 0;
}

void log_transmitted_address() {
    if ((receive_address == before_transmit_address) && (receive_address == after_transmit_address)) {
        Serial.println("     All addresses match.");
    } else {
        Serial.print("     Error. Event addresses don't match. Got 0x");
        Serial.print(receive_address, HEX);
        Serial.print(", 0x");
        Serial.print(before_transmit_address, HEX);
        Serial.print(", 0x");
        Serial.println(after_transmit_address, HEX);
    }
    transmitted = false;
}

void blink_isr() {
    led_high = !led_high;
    digitalWrite(LED_BUILTIN, led_high);
}