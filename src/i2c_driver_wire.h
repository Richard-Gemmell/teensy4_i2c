// Copyright © 2019 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef I2C_DRIVER_WIRE_H
#define I2C_DRIVER_WIRE_H

#include <Arduino.h>
#include <functional>
#include "i2c_driver.h"
#ifdef __IMXRT1062__
#include "imx_rt1060/imx_rt1060_i2c_driver.h"
#endif

class I2CDriverWire : public Stream {
public:
    // Size of RX and TX buffers. Feel free to change sizes if necessary.
    static const size_t rx_buffer_length = 32;
    static const size_t tx_buffer_length = 32;

    // Time to wait for a read or write to complete in millis
    static const uint32_t timeout_millis = 200;

    // Indicates that there is no more data to read.
    static const int no_more_bytes = -1;

    I2CDriverWire(I2CMaster& master, I2CSlave& slave)
            : Stream(), master(master), slave(slave) {
    }

    void begin() {
        end();
        master.begin(master_frequency);
    }

    void begin(int address) {
        end();
        slave.set_receive_buffer(rxBuffer, rx_buffer_length);
        slave.after_receive(std::bind(&I2CDriverWire::on_receive_wrapper, this, std::placeholders::_1));
        slave.before_transmit(std::bind(&I2CDriverWire::before_transmit, this));
        slave.listen((uint16_t)address);
    }

    void end() {
        master.end();
        slave.stop_listening();
    }

    void setClock(uint32_t frequency) {
        master_frequency = frequency;
    }

    void beginTransmission(int address) {
        write_address = (uint16_t)address;
        tx_next_byte_to_write = 0;
    }

    uint8_t endTransmission(int stop = true) {
        if (tx_next_byte_to_write > 0) {
            master.write_async(write_address, tx_buffer, tx_next_byte_to_write, stop);
            finish();
        }
        return toWireResult(master.error());
    }

    size_t write(uint8_t data) override {
        if (tx_next_byte_to_write < tx_buffer_length) {
            tx_buffer[tx_next_byte_to_write++] = data;
            return 1;
        }
        return 0;
    }

    size_t write(const uint8_t* data, size_t length) override {
        size_t avail = tx_buffer_length - tx_next_byte_to_write;
        if (avail >= length) {
            uint8_t* dest = tx_buffer + tx_next_byte_to_write;
            memcpy(dest, data, length);
            tx_next_byte_to_write += length;
            return length;
        }
        return 0;
    }

    uint8_t requestFrom(int address, int quantity, int stop = true) {
        rx_bytes_available = quantity;
        rx_next_byte_to_read = 0;
        master.read_async(address, rxBuffer, min((size_t)quantity, rx_buffer_length), stop);
        finish();
        return 0;
    }

    inline int available() override {
        return (int)(rx_bytes_available - rx_next_byte_to_read);
    }

    int read() override {
        if (rx_next_byte_to_read < rx_bytes_available) {
            return rxBuffer[rx_next_byte_to_read++];
        }
        return no_more_bytes;
    }

    int peek() override {
        if (rx_next_byte_to_read < rx_bytes_available) {
            return rxBuffer[rx_next_byte_to_read];
        }
        return no_more_bytes;
    }

    inline void onReceive(void (* function)(int len)) {
        on_receive = function;
    }

    // A callback that's called by the I2C driver's interrupt
    // service routine (ISR).
    // WARNING: This method is called inside an ISR so it must be
    // very, very fast. Avoid using it if at all possible.
    // In particular, don't call write() in this method to prepare
    // the transmit buffer. It's much better to fill the transmit
    // buffer during loop().
    inline void onRequest(void (* function)()) {
        on_request = function;
    }

    using Print::write;

private:
    I2CMaster& master;
    I2CSlave& slave;
    uint32_t master_frequency = 100 * 1000U;

    void (* on_receive)(int len) = nullptr;
    void (* on_request)() = nullptr;

    uint16_t write_address = 0;
    uint8_t tx_buffer[tx_buffer_length] = {};
    size_t tx_next_byte_to_write = 0;

    uint8_t rxBuffer[rx_buffer_length] = {};
    size_t rx_bytes_available = 0;
    size_t rx_next_byte_to_read = 0;

    static int toWireResult(I2CError status) {
        if (status == I2CError::ok) return 0;
        if (status == I2CError::buffer_overflow) return 1;
        if (status == I2CError::address_nak) return 2;
        if (status == I2CError::data_nak) return 3;
        return 4;
    }

    // Gives the application a chance to set up the transmit buffer
    // during the ISR.
    void before_transmit() {
        tx_next_byte_to_write = 0;
        if (on_request) {
            on_request();
        }
        slave.set_transmit_buffer(tx_buffer, tx_next_byte_to_write);
    }

    void finish() {
        elapsedMillis timeout;
        while (timeout < timeout_millis) {
            if (master.finished()) {
                return;
            }
        }
        Serial.println("Timed out waiting for transfer to finish.");
    }

    inline void on_receive_wrapper(size_t num_bytes) {
        rx_bytes_available = num_bytes;
        rx_next_byte_to_read = 0;
        if (on_receive) {
            on_receive(num_bytes);
        }
    }
};

I2CDriverWire Wire(Master, Slave);
I2CDriverWire Wire1(Master1, Slave1);
I2CDriverWire Wire2(Master2, Slave2);

#endif //I2C_DRIVER_WIRE_H
