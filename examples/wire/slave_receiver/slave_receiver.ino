// Wire Slave Receiver
// by Nicholas Zambetti <http://www.zambetti.com>
// Modified by Richard Gemmell Oct 2019

// Demonstrates use of the Wire library
// Receives data as an I2C/TWI slave device
// Refer to the "Wire Master Writer" example for use with this
// To use it, connect a master to the Teensy on pins 68 and 17.
//
// Consider using the I2CRegisterSlave class instead of Wire to
// create an I2C slave.

// Created 29 March 2006

// This example code is in the public domain.

#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

void receiveEvent(int howMany);

int led = LED_BUILTIN;

void setup()
{
  pinMode(led, OUTPUT);
  Wire1.begin(9);                // join i2c bus with address #9
  Wire1.onReceive(receiveEvent); // register event
  Serial.begin(9600);           // start serial for output
}

void loop()
{
  delay(100);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  digitalWrite(led, HIGH);       // briefly flash the LED
  while(Wire1.available() > 1) {  // loop through all but the last
    char c = Wire1.read();        // receive byte as a character
    Serial.print(c);             // print the character
  }
  Serial.println();
  int x = Wire1.read();           // receive byte as an integer
  Serial.println(x);             // print the integer
  digitalWrite(led, LOW);
}
