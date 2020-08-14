// Copyright © 2019-2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef I2C_REGISTER_SLAVE_H
#define I2C_REGISTER_SLAVE_H

#include <functional>
#include <utility>
#include "i2c_driver.h"

#ifdef __IMXRT1062__
#include "imx_rt1060/imx_rt1060_i2c_driver.h"
#endif

// Needs to be as big as the largest register plus 1 byte for the registry number.
#define REG_SLAVE_WRITE_BUFFER_LENGTH 9

class II2CRegisterSlave {
public:
    // Calls listen() on the underlying slave driver and then attaches our event
    // handlers. Don't call listen on the underlying slave directly or it won't work.
    virtual void listen(uint8_t address) = 0;

    // Add a callback to be notified when the master has read a register.
    // This is often used to clear the "new data available" flag if
    // there is one.
    virtual void after_read(std::function<void(uint8_t the_register)> callback) = 0;

    // Add a callback to be notified when the master has written to a
    // register.
    virtual void after_write(std::function<void(uint8_t the_register, size_t num_bytes)> callback) = 0;
};

// Wraps I2CSlave to make it easy to implement an I2C slave whose interface
// is defined as a set of "registers" that the master can read or write.
//
// The application should update the slave's read_only buffers in the main
// loop. The master is expected to read the registers asynchronously without
// the application needing to create an interrupt service routine to respond
// to the master.
//
// This class is intended to represent a single I2C device so it does not
// support multiple slave addresses.
//
// WARNING: This class registers callbacks on the I2CSlave instance that
// it wraps. e.g. I2CSlave::after_receive(). If you replace these with your
// own then you'll break I2CRegisterSlave.
class I2CRegisterSlave : public II2CRegisterSlave {
public:
    // 'mutable_buffer' a set of registers that the master can write to
    // as well as read. Normally used to configure the slave. Use a zero
    // length buffer if it's not required. Application code should avoid
    // modifying these buffers to avoid the risk of missing a change by
    // the master.
    // 'read_only_buffer' a set of registers that can be read by the master
    // but not written to. These are usually updated in the main application
    // loop. The master is free to read them whenever it pleases.
    I2CRegisterSlave(I2CSlave& slave,
                     uint8_t* mutable_buffer, size_t num_mutable_bytes,
                     uint8_t* read_only_buffer, size_t num_read_only_bytes)
            : slave(slave),
              mutable_buffer(mutable_buffer), mutable_buffer_size(num_mutable_bytes),
              read_only_buffer(read_only_buffer), read_only_buffer_size(num_read_only_bytes) {
    }

    // Calls listen() on the underlying slave driver and then attaches our event
    // handlers. Don't call listen on the underlying slave directly or it won't work.
    void listen(uint8_t address) override;

    // Add a callback to be notified when the master has read a register.
    // This is often used to clear the "new data available" flag if
    // there is one.
    //
    // WARNING: This callback is called from an interrupt service routine.
    // The callback needs to be as fast as possible and must not perform any IO.
    inline void after_read(std::function<void(uint8_t the_register)> callback) override {
        after_read_callback = std::move(callback);
    }

    // Add a callback to be notified when the master has written to a
    // register.
    //
    // WARNING: This callback is called from an interrupt service routine.
    // The callback needs to be as fast as possible and must not perform any IO.
    inline void after_write(std::function<void(uint8_t the_register, size_t num_bytes)> callback) override {
        after_write_callback = std::move(callback);
    }

private:
    I2CSlave& slave;
    uint8_t rx_buffer[REG_SLAVE_WRITE_BUFFER_LENGTH] = {};
    bool got_reg_num = false;
    uint8_t* const mutable_buffer;
    const size_t mutable_buffer_size;
    uint8_t* const read_only_buffer;
    const size_t read_only_buffer_size;
    std::function<void(uint8_t the_register)> after_read_callback = nullptr;
    std::function<void(uint8_t the_register, size_t num_bytes)> after_write_callback = nullptr;

    void after_receive(int len);

    inline void after_transmit() {
        wait_for_reg_num();
        if (after_read_callback) {
            after_read_callback(rx_buffer[0]);
        }
    }

    inline void wait_for_reg_num() {
        got_reg_num = false;
        slave.set_receive_buffer(rx_buffer, REG_SLAVE_WRITE_BUFFER_LENGTH);
        slave.set_transmit_buffer(mutable_buffer, 0);
    }
};

#endif //I2C_REGISTER_SLAVE_H
