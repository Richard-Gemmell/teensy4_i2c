// Copyright © 2019-2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#include <cstring>
#include "i2c_register_slave.h"

void I2CRegisterSlave::listen(uint8_t address) {
    slave.listen(address);
    slave.after_receive(std::bind(&I2CRegisterSlave::after_receive, this, std::placeholders::_1));
    slave.after_transmit(std::bind(&I2CRegisterSlave::after_transmit, this));
    wait_for_reg_num();
}

void I2CRegisterSlave::after_receive(int len) {
    uint8_t num_bytes = len;
    got_reg_num = !got_reg_num;
    if (got_reg_num) {
        uint8_t reg_num = rx_buffer[0];
        if (len > 1) {
            // Master sent register number and data to write to a buffer in one go
            if (reg_num < mutable_buffer_size) {
                uint8_t* buffer = mutable_buffer + reg_num;
                uint8_t buffer_size = mutable_buffer_size - reg_num;
                size_t copy_len = len - 1;
                if (copy_len > buffer_size) {
                    copy_len = buffer_size;
                }
                memcpy(buffer, rx_buffer + 1, copy_len);
            }
            num_bytes = len - 1;
            // else it's not a valid registry. Ignore it.
            got_reg_num = false;
        } else {
            if (reg_num < mutable_buffer_size) {
                // The coming read or write is aimed at the mutable buffer.
                uint8_t* buffer = mutable_buffer + reg_num;
                uint8_t buffer_size = mutable_buffer_size - reg_num;
                slave.set_receive_buffer(buffer, buffer_size);
                slave.set_transmit_buffer(buffer, buffer_size);
            } else {
                // reg_num is too big for a write. Discard the next write if there is one.
                slave.set_receive_buffer(rx_buffer, 0);

                uint8_t offset = reg_num - mutable_buffer_size;
                if (offset < read_only_buffer_size) {
                    slave.set_transmit_buffer(read_only_buffer + offset, read_only_buffer_size - offset);
                } else {
                    // reg_num is too big for a write. The next read will get dummy data.
                    slave.set_transmit_buffer(read_only_buffer, 0);
                }
            }
        }
    }
    if (!got_reg_num) {
        if (after_write_callback) {
            after_write_callback(rx_buffer[0], num_bytes);
        }
        wait_for_reg_num();
    }
}
