// Copyright © 2019-2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#include "i2c_driver_wire.h"

static int toWireResult(I2CError error) {
    if (error == I2CError::ok) return 0;
    if (error == I2CError::buffer_overflow) return 1;
    if (error == I2CError::address_nak) return 2;
    if (error == I2CError::data_nak) return 3;
    return 4;
}

I2CDriverWire::I2CDriverWire(I2CMaster& master, I2CSlave& slave)
        : Stream(), master(master), slave(slave) {
}

void I2CDriverWire::setClock(uint32_t frequency) {
    master_frequency = frequency;
}

void I2CDriverWire::begin() {
    end();
    master.begin(master_frequency);
}

void I2CDriverWire::begin(uint8_t address) {
    prepare_slave();
    slave.listen(address);
}

void I2CDriverWire::begin(uint8_t first_address, uint8_t second_address) {
    prepare_slave();
    slave.listen(first_address, second_address);
}

void I2CDriverWire::beginRange(uint8_t first_address, uint8_t last_address) {
    prepare_slave();
    slave.listen_range(first_address, last_address);
}

void I2CDriverWire::prepare_slave() {
    end();
    slave.set_receive_buffer(rxBuffer, rx_buffer_length);
    slave.after_receive(std::bind(&I2CDriverWire::on_receive_wrapper, this, std::placeholders::_1, std::placeholders::_2));
    slave.before_transmit(std::bind(&I2CDriverWire::before_transmit, this, std::placeholders::_1));
}

void I2CDriverWire::end() {
    master.end();
    slave.stop_listening();
}

void I2CDriverWire::beginTransmission(int address) {
    write_address = (uint8_t)address;
    tx_next_byte_to_write = 0;
}

uint8_t I2CDriverWire::endTransmission(int stop) {
    master.write_async(write_address, tx_buffer, tx_next_byte_to_write, stop);
    finish();
    return toWireResult(master.error());
}

size_t I2CDriverWire::write(uint8_t data) {
    if (tx_next_byte_to_write < tx_buffer_length) {
        tx_buffer[tx_next_byte_to_write++] = data;
        return 1;
    }
    return 0;
}

size_t I2CDriverWire::write(const uint8_t* data, size_t length) {
    size_t avail = tx_buffer_length - tx_next_byte_to_write;
    if (avail >= length) {
        uint8_t* dest = tx_buffer + tx_next_byte_to_write;
        memcpy(dest, data, length);
        tx_next_byte_to_write += length;
        return length;
    }
    return 0;
}

uint8_t I2CDriverWire::requestFrom(int address, int quantity, int stop) {
    rx_bytes_available = 0;
    rx_next_byte_to_read = 0;
    master.read_async((uint8_t)address, rxBuffer, min((size_t)quantity, rx_buffer_length), stop);
    finish();
    rx_bytes_available = master.get_bytes_transferred();
    return rx_bytes_available;
}

int I2CDriverWire::read() {
    if (rx_next_byte_to_read < rx_bytes_available) {
        return rxBuffer[rx_next_byte_to_read++];
    }
    return no_more_bytes;
}

int I2CDriverWire::peek() {
    if (rx_next_byte_to_read < rx_bytes_available) {
        return rxBuffer[rx_next_byte_to_read];
    }
    return no_more_bytes;
}

// Gives the application a chance to set up the transmit buffer
// during the ISR.
void I2CDriverWire::before_transmit(uint16_t address) {
    last_address_called = address;
    tx_next_byte_to_write = 0;
    if (on_request) {
        on_request();
    }
    slave.set_transmit_buffer(tx_buffer, tx_next_byte_to_write);
}

void I2CDriverWire::finish() {
    elapsedMillis timeout;
    while (timeout < timeout_millis) {
        if (master.finished()) {
            return;
        }
    }
    Serial.println("Timed out waiting for transfer to finish.");
}

void I2CDriverWire::on_receive_wrapper(size_t num_bytes, uint16_t address) {
    last_address_called = address;
    rx_bytes_available = num_bytes;
    rx_next_byte_to_read = 0;
    if (on_receive) {
        on_receive(num_bytes);
    }
}

I2CDriverWire Wire(Master, Slave);
I2CDriverWire Wire1(Master1, Slave1);
I2CDriverWire Wire2(Master2, Slave2);
