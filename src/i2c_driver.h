// Copyright © 2019-2023 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <cstdint>
#include <cstddef>
#include <functional>

enum class I2CError {
    // 'ok' means there were no errors since the last transaction started
    ok = 0,

    // Remaining codes mean that something went wrong.
    arbitration_lost = 1,       // Another master interrupted
    buffer_overflow = 2,        // Raised by the slave if the master sends too many bytes to fit in the receive buffer. The extra bytes are dropped.
    buffer_underflow = 3,       // Raised by the slave if the master requires more bytes than the transmit buffer holds. Slave pads the message with 0x00.
    invalid_request = 4,        // Caller asked Master to read more than 256 bytes in one go
    master_pin_low_timeout = 5, // SCL or SDA held low for too long. Can be caused by a stuck slave.
    master_not_ready = 6,       // Caller failed to wait for one transaction to finish before starting the next
    master_fifo_error = 7,      // Master attempted to send or receive without a START. Programming error.
    master_fifos_not_empty = 8, // Programming error. FIFOs not empty at start of transaction.
    address_nak = 9,            // Raised by master if the slave does not reply when called
    data_nak = 10,              // Raised by master if the slave fails to acknowledge a data byte
    bit_error = 11              // Slave sent a 1 but found a 0 on the bus. Transaction aborted.
};

enum class InternalPullup : uint32_t {
    // Disables the internal pullup resistors.
    // This setting is useful if you want to use external pullups and you
    // want to keep the maths simple.
    disabled =  0,

    // The 22k internal pullup resistors are enabled by default.
    // They may be sufficient for a 100k bus especially if another device also has
    // internal pullups. You'll need to use external pullups for faster lines.
    enabled_22k_ohm =  3,

    // The 47k and 100k internal pullup options are here for completeness.
    // They're too weak to use without external pullups.
    enabled_47k_ohm =  1,
    enabled_100k_ohm =  2,
};

// Contains behaviour that's common to both masters and slaves.
class I2CDriver {
public:
    explicit I2CDriver();

    // Indicates whether the driver is working or what happened
    // in the last read/write
    inline I2CError error() {
        return _error;
    }

    // True if the last operation failed. i.e. if last_error returns anything other than ok.
    inline bool has_error() {
        return _error > I2CError::ok;
    }

    // Sets the pad control configuration that will be used for the I2C pins.
    // This sets the drive strength, hysteresis etc.
    // This change takes effect the next time you call begin() or listen().
    // The internal pullup resistor is configured by set_internal_pullup().
    //
    // The default value is PAD_CONTROL_CONFIG defined in imx_rt1060_i2c_driver.cpp.
    // You may need to override the default implementation to tune the pad driver's
    // impedance etc. See imx_rt1060_i2c_driver.cpp for details.
    inline void set_pad_control_configuration(uint32_t config) {
        pad_control_config = config;
    }

    // Enables or disables the internal pullup resistors.
    // You may need external pullups even if you enable the internal ones.
    // This change takes effect the next time you call begin() or listen().
    inline void set_internal_pullups(InternalPullup pullup) {
        pullup_config = pullup;
    }

protected:
    volatile I2CError _error = I2CError::ok;
    uint32_t pad_control_config;
    InternalPullup pullup_config;
};

class I2CMaster : public I2CDriver {
public:
    // Configures the master and enables it. You should call this before
    // attempting to communicate with any slaves.
    // 'frequency' determines the bus frequency in Hz. You must not set
    // the speed higher than the highest speed of the slowest device on the bus.
    // 100 kHz will work with any I2C device. Other common values are 400 kHz and 1 MHz.
    // Returns true if the bus was acquired successfully.
    virtual void begin(uint32_t frequency) = 0;

    // Makes the master release all resources that it gathered in begin()
    // You only need to call this if you want to use the same pins for
    // something else.
    virtual void end() = 0;

    // False while the driver is doing work. e.g. reading or writing
    // True when it's Ok to do another read or write
    virtual bool finished() = 0;

    // Returns the number of bytes transferred by the last call to
    // write_async or read_async.
    virtual size_t get_bytes_transferred() = 0;

    // Transmits the contents of buffer to the slave at the given address.
    // The caller must not modify the buffer until the read is complete.
    // Set 'num_bytes' to 0 to find out if there's a slave listening on this address.
    // Set 'send_stop' to true if this is the last transfer in the transaction.
    // Set 'send_stop' to false if are going to make another transfer.
    // Call finished() to see if the call has finished.
    virtual void write_async(uint8_t address, const uint8_t* buffer, size_t num_bytes, bool send_stop) = 0;

    // Reads the specified number of bytes and copies them into the supplied buffer.
    // The caller must not modify the buffer until the read is complete.
    // Set 'num_bytes' to 0 to find out if there's a slave listening on this address.
    // Set 'send_stop' to true if this is the last transfer in the transaction.
    // Set 'send_stop' to false if are going to make another transfer.
    // Call finished() to see if the call has finished.
    virtual void read_async(uint8_t address, uint8_t* buffer, size_t num_bytes, bool send_stop) = 0;
};

class I2CSlave : public I2CDriver {
public:
    // Start listening to the master on the given address. Makes the slave visible on the bus.
    virtual void listen(uint8_t address) = 0;

    // Like listen(uint8_t) except that the slave will listen on 2 different I2C addresses.
    // This makes it appear as 2 devices to the master.
    virtual void listen(uint8_t first_address, uint8_t second_address) = 0;

    // Like listen(uint8_t) except that the slave will listen on every address
    // in the range from 'first_address' to 'last_address' inclusive.
    virtual void listen_range(uint8_t first_address, uint8_t last_address) = 0;

    // Detach from the bus. The slave will no longer be visible to the master.
    // Does nothing unless the slave is listening.
    virtual void stop_listening() = 0;

    // Sets a callback to be called by the ISR each time
    // the slave receives a block of data from the master.
    // 'length' is the number of bytes that were received by the slave
    // 'address' is the address that the master called. This is only
    // useful when the slave is listening on multiple addresses.
    //
    // Set 'callback' to 'nullptr' to remove the previous callback.
    virtual void after_receive(std::function<void(size_t length, uint16_t address)> callback) = 0;

    // Sets a callback to be called by the ISR just before
    // the slave transmits a block of data to the master.
    // 'address' is the address that the master called. This is only
    // useful when the slave is listening on multiple addresses.
    //
    // Set 'callback' to 'nullptr' to remove the previous callback.
    virtual void before_transmit(std::function<void(uint16_t address)> callback) = 0;

    // Sets a callback to be called by the ISR each time the slave
    // has sent a block of data to the master.
    // 'address' is the address that the master called. This is only
    // useful when the slave is listening on multiple addresses.
    //
    // Set 'callback' to 'nullptr' to remove the previous callback.
    virtual void after_transmit(std::function<void(uint16_t address)> callback) = 0;

    // Determines which data will be sent to the master the next time
    // it reads from us.
    // The master will receive up to 'size' bytes from us. If it demands
    // more data it will be sent 0x00 until it ends the request.
    virtual void set_transmit_buffer(const uint8_t* buffer, size_t size) = 0;

    // Determines where to put data we receive from the master
    // the next time it writes to us.
    // The master can send up to 'size' bytes of data to us. Extra bytes
    // will be dropped. The 'after_receive' callback receives the actual number
    // of bytes received in the 'length' argument.
    //
    // Note that the slave does not send NACK when the buffer is full.
    // It just ignores the extra data.
    virtual void set_receive_buffer(uint8_t* buffer, size_t size) = 0;
};

#endif //I2C_DRIVER_H
