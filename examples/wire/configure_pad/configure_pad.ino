// Copyright © 2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// This example shows the use of set_pad_control_configuration() to
// change the Teensy's pad configuration. This may be necessary if
// your project has an unusual I2C setup. e.g. if you need to disable
// the Teensy's internal pullup resistor or if you need to match the
// Teeensy's impedance to the rest of the I2C circuit. See
// imx_rt1060_i2c_driver.cpp for more information.
//
// This example WILL NOT work unless you have an INA260
// current sensor connected to pins 17 and 18.

#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

int led = LED_BUILTIN;

// Define the pad control configuration that we wish to use.
// This happens to be the config that was used prior to version 0.9.5 of teensy4_i2c
#define PAD_CONFIG IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SPEED(2) | IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(3)

void setup()
{
  pinMode(led, OUTPUT);

  // This call to setPadControlConfiguration() overrides the default
  // configuration defined in imx_rt1060_i2c_driver.cpp.
  Wire.setPadControlConfiguration(PAD_CONFIG);
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
