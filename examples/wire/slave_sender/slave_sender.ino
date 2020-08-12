// Wire Slave Sender
// by Nicholas Zambetti <http://www.zambetti.com>
// Modified by Richard Gemmell Oct 2019

// Demonstrates use of the Wire library
// Sends data as an I2C/TWI slave device
// Refer to the "Wire Master Reader" example for use with this
// To use it, connect a master to the Teensy on pins 16 and 17.
//
// Consider using the I2CRegisterSlave class instead of Wire to
// create an I2C slave.

// Created 29 March 2006

// This example code is in the public domain.

#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

void requestEvent();

int led = LED_BUILTIN;

void setup()
{
  pinMode(led, OUTPUT);
  Wire1.begin(8);        // join i2c bus with address #8
  Wire1.onRequest(requestEvent); // register event
}

void loop()
{
  delay(100);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  digitalWrite(led, HIGH);      // briefly flash the LED
  Wire1.write("hello ");     // respond with message of 6 bytes
                                // as expected by master
  digitalWrite(led, LOW);
}
