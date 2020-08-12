// Copyright © 2019 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

// Demonstrates use of the I2C Device class to represent a slave device.
// Creates an I2C master, configures a device and reads registers.
//
// To use it, connect a master to the Teensy on pins 16 and 17.
// Use the master to read registers 0 to 10.
// The master can write registers 0 and 1 to change the output.
// loop() updates the output values. Changes to Settings will
// be applied the next time loop() runs.
//
// I recommend this approach for using the Teensy as an I2C slave.

#include <Arduino.h>
#include <i2c_register_slave.h>

volatile bool led_high = false;

// Registers that the caller can both read and write
struct Settings {
    int8_t temp_offset; // Register 0. Zero point for temperature. Default -40.
    int8_t scaling;     // Register 1. Writable. Sets scaling of voltage and current fields.
};

// Registers that the caller can only read
struct Registers {
    uint8_t flags = 0;      // Register 2. bit 0 => new data available
    int8_t temp = 0;        // Register 3. degrees C starting at temp_offset
    uint16_t reserved = 0;  // Register 4. Fill up to the next word boundary
    int32_t voltage = 0;    // Register 6. in 10ths of a volt
    int32_t current = 0;    // Register 10. in 10ths of an amp
};

Settings settings = {-40, 10};
Registers registers;
I2CRegisterSlave registerSlave = I2CRegisterSlave(Slave1, (uint8_t*)&settings, sizeof(Settings), (uint8_t*)&registers, sizeof(Registers));

// The code to read the raw data might be too expensive to
// call in an interrupt service routine. It's represented
// by these dummy methods.
int8_t get_temp();
int32_t get_current();
int32_t get_voltage();

// Callbacks.
void on_read_isr(uint8_t reg_num);

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    // Start listening
    registerSlave.listen(0x2D);
    registerSlave.after_read(on_read_isr);

    // Enable the serial port for debugging
    Serial.begin(9600);
    Serial.println("Registers. 0 => temp_offset, 1 => scaling, 2 => flags, 3 => temp, 6 => voltage, 10 => current.");
}

void loop() {
    // Gather raw data and convert to output values
    Registers new_values;
    new_values.temp = get_temp() - settings.temp_offset;
    int32_t raw_voltage = get_voltage();
    new_values.voltage = raw_voltage * settings.scaling;
    int32_t raw_current = get_current();
    new_values.current = raw_current * settings.scaling;

    // Block copy new values over the top of the old values
    // and then set the "new data" bit.
    memcpy(&registers, &new_values, sizeof(Registers));
    registers.flags = 1;

    // Blink the LED
    digitalWrite(LED_BUILTIN, led_high);
    led_high = !led_high;
    delay(200); // Update at 5 Hz
}

void on_read_isr(uint8_t reg_num) {
    // Clear the "new data" bit so the master knows it's
    // already read this set of values.
    registers.flags = 0;
}

// Gets temperature in degrees
int8_t get_temp() {
    return 21;
}

// Gets current in milliamps
int32_t get_current() {
    return 1250;
}

// Gets voltage in millivolts
int32_t get_voltage() {
    return 12000;
}
