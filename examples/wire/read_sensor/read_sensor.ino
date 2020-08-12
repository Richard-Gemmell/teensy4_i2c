// Copyright © 2019-2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// A Simple Sensor
// Shows how to use a typical I2C sensor.
// The program configures the sensor and then reads
// from a register. This is a common pattern.
// To use it, connect a master to the Teensy on pins 18 and 19.
//
// Consider using the I2CDevice class instead of Wire to read a sensor.

#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

int sensor_address = 0x40;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);                    // start serial for output

  Wire.setClock(400 * 1000);   // Set the clock speed before calling begin()
  Wire.begin();                         // join i2c bus

  // Configure the slave by writing to register 0
  uint16_t new_config = 0x6763;                           // Suitable configuration values
  Serial.println(new_config, HEX);
  Wire.beginTransmission(sensor_address);
  Wire.write(0);                                      // Write to register 0
  Wire.write((uint8_t*)&new_config, sizeof(new_config)); // Send the new config
  Wire.endTransmission(true);
}

void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);  // briefly flash the LED

  // Read back the config

  // Request the contents of register 0
  Wire.beginTransmission(sensor_address);
  Wire.write(0);
  Wire.endTransmission(false);

  // Read the register
  Wire.requestFrom(sensor_address, 2, true);
  while(Wire.available()) {
    uint8_t c = Wire.read();
    Serial.print(c, HEX);   // Note that the byte order has changed!
  }

  Serial.println();
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
