// Copyright © 2019-2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef I2C_DRIVER_WIRE_H
#define I2C_DRIVER_WIRE_H

#include <Arduino.h>
#include <functional>
#include "i2c_driver.h"
#ifdef __IMXRT1062__
#include "imx_rt1060/imx_rt1060_i2c_driver.h"
#endif

// An implementation of the Wire library as defined at
//   https://www.arduino.cc/en/reference/wire
// This header also defines TwoWire as an alias for I2CDriverWire
// for better compatibility with Teensy libraries.
// WARNING. This implementation does not include extensions to
// Wire that were part of the Teensy 3 implementation. e.g. setSDA()
class I2CDriverWire : public Stream {
public:
    // Size of RX and TX buffers. Feel free to change sizes if necessary.
    static const size_t rx_buffer_length = 32;
    static const size_t tx_buffer_length = 32;

    // Time to wait for a read or write to complete in millis
    static const uint32_t timeout_millis = 200;

    // Indicates that there is no more data to read.
    static const int no_more_bytes = -1;

    I2CDriverWire(I2CMaster& master, I2CSlave& slave);

    // Sets the pad control configuration that will be used for the I2C pins.
    // This enables the built in pull up resistor and sets the pin impedance etc.
    // You must call this method before calling begin()
    //
    // The default is PAD_CONTROL_CONFIG defined in imx_rt1060_i2c_driver.cpp.
    // You may need to override the default implementation to tune the pad driver's
    // impedance etc. See imx_rt1060_i2c_driver.cpp for details.
    inline void setPadControlConfiguration(uint32_t config) {
        master.set_pad_control_configuration(config);
        slave.set_pad_control_configuration(config);
    }

    // Call setClock() before calling begin() to set the I2C frequency.
    // Although you can pass any frequency, it will be converted to one
    // of the standard values of 100_000, 400_000 or 1_000_000.
    // The default is 100000.
    void setClock(uint32_t frequency);

    // Use this version of begin() to initialise a master.
    void begin();

    // Use this version of begin() to initialise a slave.
    void begin(uint8_t address);

    // Use this version of begin() to initialise a slave that listens
    // on 2 different addresses
    void begin(uint8_t first_address, uint8_t second_address);

    // Use this version of begin() to initialise a slave that listens
    // on 2 different addresses
    void beginRange(uint8_t first_address, uint8_t last_address);

    void end();

    void beginTransmission(int address);

    uint8_t endTransmission(int stop = true);

    size_t write(uint8_t data) override;

    size_t write(const uint8_t* data, size_t length) override;

    uint8_t requestFrom(int address, int quantity, int stop = true);

    inline int available() override {
        return (int)(rx_bytes_available - rx_next_byte_to_read);
    }

    int read() override;

    int peek() override;

    // Registers a function to be called when a slave device receives
    // a transmission from a master.
    //
    // WARNING: This method is called inside the driver's interrupt
    // service routing so it must be very, very fast. In particular,
    // you should avoid doing any IO in the callback.
    inline void onReceive(void (* function)(int len)) {
        on_receive = function;
    }

    // Register a function to be called when a master requests data from
    // this slave device.
    //
    // WARNING: This method is called inside the driver's interrupt
    // service routing so it must be very, very fast. Avoid using it
    // if possible and avoid IO. In particular, don't call write()
    // in this method to prepare the transmit buffer. It's much better
    // to fill the transmit buffer during loop().
    inline void onRequest(void (* function)()) {
        on_request = function;
    }

    // Returns the address that the slave responded to the last
    // time the master accessed it. This is only useful for slaves
    // that are listening to more than one address.
    inline int getLastAddress() {
        return last_address_called;
    }

    // Override various functions to avoid ambiguous calls
    inline void begin(int address) { begin((uint8_t)address); }
    inline void begin(int first_address, int second_address) { begin((uint8_t)first_address, (uint8_t)second_address); }
    inline void beginRange(int first_address, int last_address) { beginRange((uint8_t)first_address, (uint8_t)last_address); }

    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    using Print::write;

private:
    I2CMaster& master;
    I2CSlave& slave;
    uint32_t master_frequency = 100 * 1000U;

    void (* on_receive)(int len) = nullptr;
    void (* on_request)() = nullptr;

    uint8_t write_address = 0;
    uint8_t tx_buffer[tx_buffer_length] = {};
    size_t tx_next_byte_to_write = 0;

    uint8_t rxBuffer[rx_buffer_length] = {};
    size_t rx_bytes_available = 0;
    size_t rx_next_byte_to_read = 0;

    uint16_t last_address_called = 0xFF;

    void prepare_slave();
    void before_transmit(uint16_t address);
    void finish();
    void on_receive_wrapper(size_t num_bytes, uint16_t address);
};

extern I2CDriverWire Wire;      // Pins 19 and 18; SCL0 and SDA0
extern I2CDriverWire Wire1;     // Pins 16 and 17; SCL1 and SDA1
extern I2CDriverWire Wire2;     // Pins 24 and 25; SCL2 and SDA2

// Alias for backwards compatibility with Wire.h
using TwoWire = I2CDriverWire;

#endif //I2C_DRIVER_WIRE_H
