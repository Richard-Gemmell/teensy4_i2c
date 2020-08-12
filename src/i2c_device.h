// Copyright © 2019 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef I2C_DEVICE_H
#define I2C_DEVICE_H

#include <elapsedMillis.h>
#include "i2c_driver.h"
#ifdef __IMXRT1062__
#include "imx_rt1060/imx_rt1060_i2c_driver.h"
#endif

// Wraps I2CMaster to make it easy to configure a typical I2C device and then read
// "registers". All calls block so this API should not be called within interrupt
// service routines.
class II2CDevice {
public:
    virtual bool write(uint8_t reg, uint8_t* buffer, size_t num_bytes, bool send_stop) = 0;
    virtual bool write(uint8_t reg, uint8_t value, bool send_stop) = 0;
    virtual bool write(uint8_t reg, int8_t value, bool send_stop) = 0;
    virtual bool write(uint8_t reg, uint16_t value, bool send_stop) = 0;
    virtual bool write(uint8_t reg, int16_t value, bool send_stop) = 0;
    virtual bool write(uint8_t reg, uint32_t value, bool send_stop) = 0;
    virtual bool write(uint8_t reg, int32_t value, bool send_stop) = 0;

    virtual bool read(uint8_t reg, uint8_t* buffer, size_t num_bytes, bool send_stop) = 0;
    virtual bool read(uint8_t reg, uint8_t* value, bool send_stop) = 0;
    virtual bool read(uint8_t reg, int8_t* value, bool send_stop) = 0;
    virtual bool read(uint8_t reg, uint16_t* value, bool send_stop) = 0;
    virtual bool read(uint8_t reg, int16_t* value, bool send_stop) = 0;
    virtual bool read(uint8_t reg, uint32_t* value, bool send_stop) = 0;
    virtual bool read(uint8_t reg, int32_t* value, bool send_stop) = 0;
};

// Wraps I2CMaster to make it easy to configure a typical I2C device and then read
// "registers". All calls block so this API should not be called within interrupt
// service routines.
class I2CDevice : public II2CDevice {
public:
    const uint32_t timeout_millis = 200;

    I2CDevice(I2CMaster& master, uint8_t address, int device_byte_order = _LITTLE_ENDIAN)
            : master(master), address(address) {
        swap_bytes = _BYTE_ORDER != device_byte_order;
    }

    bool write(uint8_t reg, uint8_t* buffer, size_t num_bytes, bool send_stop) override {
        uint8_t big_buffer[num_bytes+1];
        big_buffer[0] = reg;
        memcpy(big_buffer+1, buffer, num_bytes);
        master.write_async(address, big_buffer, sizeof(big_buffer), send_stop);
        finish();
        return !master.has_error();
    }

    inline bool write(uint8_t reg, uint8_t value, bool send_stop) override {
        return write(reg, &value, 1, send_stop);
    }

    inline bool write(uint8_t reg, int8_t value, bool send_stop) override {
        return write(reg, (uint8_t*)&value, 1, send_stop);
    }

    inline bool write(uint8_t reg, uint16_t value, bool send_stop) override {
        if (swap_bytes) {
            uint16_t swapped = __builtin_bswap16(value);
            return write(reg, (uint8_t*)&swapped, 2, send_stop);
        } else {
            return write(reg, (uint8_t*)&value, 2, send_stop);
        }
    }

    inline bool write(uint8_t reg, int16_t value, bool send_stop) override {
        if (swap_bytes) {
            int16_t swapped = __builtin_bswap16(value);
            return write(reg, (uint8_t*)&swapped, 2, send_stop);
        } else {
            return write(reg, (uint8_t*)&value, 2, send_stop);
        }
    }

    inline bool write(uint8_t reg, uint32_t value, bool send_stop) override {
        if (swap_bytes) {
            uint32_t swapped = __builtin_bswap32(value);
            return write(reg, (uint8_t*)&swapped, 4, send_stop);
        } else {
            return write(reg, (uint8_t*)&value, 4, send_stop);
        }
    }

    inline bool write(uint8_t reg, int32_t value, bool send_stop) override {
        if (swap_bytes) {
            int32_t swapped = __builtin_bswap32(value);
            return write(reg, (uint8_t*)&swapped, 4, send_stop);
        } else {
            return write(reg, (uint8_t*)&value, 4, send_stop);
        }
    }

    bool read(uint8_t reg, uint8_t* buffer, size_t num_bytes, bool send_stop) override {
        bool has_error = true;
        if (write_register(reg)) {
            master.read_async(address, buffer, num_bytes, send_stop);
            finish();
            has_error = master.has_error();
        }
        if (has_error) {
            // Zero the buffer if the read failed to avoid using stale data.
            memset(buffer, 0, num_bytes);
        }
        return !has_error;
    }

    inline bool read(uint8_t reg, uint8_t* value, bool send_stop) override {
        return read(reg, value, 1, send_stop);
    }

    inline bool read(uint8_t reg, int8_t* value, bool send_stop) override {
        return read(reg, (uint8_t*)value, 1, send_stop);
    }

    inline bool read(uint8_t reg, uint16_t* value, bool send_stop) override {
        if (read(reg, (uint8_t*)value, 2, send_stop)) {
            if (swap_bytes) {
                *value = __builtin_bswap16(*value);
            }
            return true;
        }
        return false;
    }

    inline bool read(uint8_t reg, int16_t* value, bool send_stop) override {
        return read(reg, (uint16_t*)value, send_stop);
    }

    inline bool read(uint8_t reg, uint32_t* value, bool send_stop) override {
        if (read(reg, (uint8_t*)value, 4, send_stop)) {
            if (swap_bytes) {
                *value = __builtin_bswap32(*value);
            }
            return true;
        }
        return false;
    }

    inline bool read(uint8_t reg, int32_t* value, bool send_stop) override {
        return read(reg, (uint32_t*)value, send_stop);
    }

private:
    I2CMaster& master;
    uint8_t address;
    bool swap_bytes;

    bool write_register(uint8_t reg) {
        master.write_async(address, &reg, 1, false);
        finish();
        return !master.has_error();
    }

    void finish() {
        elapsedMillis timeout;
        while (timeout <= timeout_millis) {
            if (master.finished()) {
                return;
            }
        }
    }
};

#endif //I2C_DEVICE_H
