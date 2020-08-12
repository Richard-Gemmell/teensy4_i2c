// Copyright © 2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// Shows how an I2C slave can listen to more than one address.
// This is handy if you want the Teensy to impersonate a lot of different devices.
//
// To use it, connect a master to the Teensy on pins 16 and 17.
// Use the master to query the Teensy on different addresses.
// The Teensy echoes the I2C address back to the master.

#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

// Blink the LED to make sure the Teensy hasn't hung
IntervalTimer blink_timer;
volatile bool led_high = false;
void blink_isr();

// The I2C port we're going to use
I2CDriverWire wire = Wire1;

// The addresses that master called
volatile uint16_t receive_address = 0xFF;
volatile uint16_t request_address = 0xFF;
volatile bool handled_request = false;

// Wire callbacks
void onReceiveHandler(int num_bytes);
void onRequestHandler();

void setup() {
  // Turn the LED on
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, true);

  // Create a timer to blink the LED
  blink_timer.begin(blink_isr, 500000);

  // Configure I2C Slave
  wire.onReceive(onReceiveHandler);
  wire.onRequest(onRequestHandler);

  // Listen to every addresses in the range
  wire.beginRange(0x10, 0x20);

  // Use this call to listen() if you want to listen to 2 addresses
  // without listening to every address in the range.
//  wire.begin(0x10, 0x20);

  // Enable the serial port for debugging
  Serial.begin(9600);
  Serial.println("Started");
}

void loop() {
    if (handled_request) {
        if (receive_address != request_address) {
            Serial.println("Error: Did not get addresses correctly.");
        } else {
            Serial.print("Handled request on address 0x");
            Serial.println(request_address, HEX);
        }
        handled_request = false;
    }

    delay(2);
}

void onReceiveHandler(int num_bytes) {
    receive_address = wire.getLastAddress();
}

void onRequestHandler() {
    request_address = wire.getLastAddress();
    wire.write(wire.getLastAddress());
    handled_request = true;
}

void blink_isr() {
    led_high = !led_high;
    digitalWrite(LED_BUILTIN, led_high);
}