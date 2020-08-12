// Wire Master Reader
// by Nicholas Zambetti <http://www.zambetti.com>
// Modified by Richard Gemmell Oct 2019

// Demonstrates use of the Wire library
// Reads data from an I2C/TWI slave device connected to pins 18 and 19.
//
// Consider using the I2CDevice class instead of Wire to read a sensor.

// Created 29 March 2006

// This example code is in the public domain.


#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

int led = LED_BUILTIN;

void setup()
{
  pinMode(led, OUTPUT);
  Wire.begin();                         // join i2c bus
  Serial.begin(9600);                    // start serial for output
}

void loop()
{
  Serial.print("read: ");

  digitalWrite(led, HIGH);  // briefly flash the LED
  Wire.requestFrom(0x40, 2);   // request 2 bytes from slave device #64

  // Can peek at the first byte
  if (Wire.available()) {
      Serial.print((char)Wire.peek());
      Serial.print(" ");
  }

  while(Wire.available()) { // slave may send less than requested
    char c = Wire.read();   // receive a byte as character
    Serial.print(c);         // print the character
  }

  Serial.println();
  delay(500);
  digitalWrite(led, LOW);
  delay(500);
}
