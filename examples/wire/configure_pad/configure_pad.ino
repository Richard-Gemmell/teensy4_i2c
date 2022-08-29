// Copyright © 2020-2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// This example shows the use of setPadControlConfiguration() to
// change the Teensy's pad configuration. You don't normally need
// to do this. This is only necessary if you need to change the pin's
// drive strength etc. See imx_rt1060_i2c_driver.cpp for more information.
//
// This example WILL NOT work unless you have an INA260 current sensor
// Connect the INA260's SCL and SDA pins to Teensy pins 19 and 18.

#include <Arduino.h>
#include <i2c_driver_wire.h>

// Define the pad control configuration that we wish to use.
// This happens to be the config that was used prior to version 0.9.5 of teensy4_i2c
#define PAD_CONFIG (IOMUXC_PAD_ODE | IOMUXC_PAD_DSE(6) | IOMUXC_PAD_SPEED(2))

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    // This call to setPadControlConfiguration() overrides the default
    // configuration defined in imx_rt1060_i2c_driver.cpp.
    Wire.setPadControlConfiguration(PAD_CONFIG);
    Wire.begin();       // join i2c bus

    Serial.begin(9600); // start serial for output
}

void loop()
{
    // Read 2 bytes from the slave device at address 64.
    Wire.requestFrom(0x40, 2);

    // Print the response.
    if (Wire.available()) {
        Serial.printf("Expected 0x6127. Got 0x%X%X.", Wire.read(), Wire.read());
    }
    Serial.println();

    // Blink the LED to show the Teensy is alive.
    digitalToggle(LED_BUILTIN);
    delay(1000);
}
