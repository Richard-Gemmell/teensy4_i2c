// Copyright © 2019-2020 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef IMX_RT1060_I2C_DRIVER_H
#define IMX_RT1060_I2C_DRIVER_H

#include <cstdint>
#include <imxrt.h>
#include "imx_rt1060.h"
#include "../i2c_driver.h"

// A read or write buffer.
// You cannot use the same buffer for both reading and writing.
// This class is an implementation detail of the driver and
// should not be used elsewhere.
class I2CBuffer {
public:
    I2CBuffer();

    // May be called inside or outside the ISR.
    // This should be safe because it can never be called while
    // the ISR is trying to read from the buffer or write to it.
    inline void initialise(uint8_t* new_buffer, size_t new_size) {
        reset();
        buffer = new_buffer;
        size = new_size;
    }

    inline bool initialised() {
        return size > 0;
    }

    // Empties the buffer.
    inline void reset() {
        next_index = 0;
    }

    // Returns 'false' if the buffer was already full.
    inline bool write(uint8_t data) {
        if (next_index == size) {
            return false;
        } else {
            buffer[next_index++] = data;
            return true;
        }
    }

    inline size_t get_bytes_transferred() {
        return next_index;
    }

    // Caller is responsible for preventing a read beyond the end of the buffer.
    inline uint8_t read() {
        return buffer[next_index++];
    }

    inline bool not_started_writing() {
        return next_index == 0;
    }

    inline bool finished_writing() {
        return next_index == size;
    }

    inline bool not_started_reading() {
        return next_index == 0;
    }

    inline bool finished_reading() {
        return next_index == size;
    }

    inline bool has_data_available() {
        return next_index < size;
    }

private:
    volatile uint8_t* buffer;
    volatile size_t size = 0;
    volatile size_t next_index = 0;
};

class IMX_RT1060_I2CBase {
public:
    typedef struct {
        const uint8_t pin;          // The Teensy 4.0 pin number
        const uint32_t mux_val;     // Value to set for mux;
        volatile uint32_t* select_input_register; // Which register controls the selection
        const uint32_t select_val;  // Value for that selection
    } PinInfo;

    typedef struct {
        volatile uint32_t& clock_gate_register;
        uint32_t clock_gate_mask;
        PinInfo sda_pin;             // The default SDA pin
        PinInfo scl_pin;             // The default SCL pin
        bool has_alternatives;       // True if there are alternative pins
        PinInfo alternative_sda_pin; // The alternative SDA pin. Undefined if has_alternatives is false
        PinInfo alternative_scl_pin; // The alternative SCL pin. Undefined if has_alternatives is false
        IRQ_NUMBER_t irq;            // The interrupt request number for this port
    } Config;
};

class IMX_RT1060_I2CMaster : public I2CMaster {
public:
    IMX_RT1060_I2CMaster(IMXRT_LPI2C_Registers* port, IMX_RT1060_I2CBase::Config& config, void (* isr)());

    // Supports the following frequencies:
    //    100,000 - Standard Mode - up to 100 kHz
    //    400,000 - Fast Mode - up to 400 kHz
    //   1000,000 - Fast Mode Plus - up to 1 MHz
    // If you set frequency to any other value the system will use
    // the fastest mode which is slower than the given value.
    // e.g. Setting frequency to 200,000 will give you a 100 kHz bus.
    void begin(uint32_t frequency) override;

    void end() override;

    bool finished() override;

    size_t get_bytes_transferred() override;

    void write_async(uint8_t address, uint8_t* buffer, size_t num_bytes, bool send_stop) override;

    void read_async(uint8_t address, uint8_t* buffer, size_t num_bytes, bool send_stop) override;

    // DO NOT call this method directly.
    void _interrupt_service_routine();
private:
    enum class State {
        // Busy states
        starting = 0,       // Waiting for START to be sent and acknowledged
        transferring,       // In a transfer
        stopping,           // Transfer complete or aborted. Waiting for STOP.

        // 'idle' and above mean that the driver has finished
        // whatever it was doing and is ready to do more work.
        idle = 100,         // Not in a transaction
        transfer_complete,  // Transfer has finished and caller has not requested a STOP.
        stopped             // Transaction has finished. STOP sent.
    };

    IMXRT_LPI2C_Registers* const port;
    IMX_RT1060_I2CBase::Config& config;
    I2CBuffer buff = I2CBuffer();
    volatile State state = State::idle;
    volatile uint32_t ignore_tdf = false;          // True for a receivve transfer
    volatile bool stop_on_completion = false;   // True if the transmit transfer requires a stop.

    void (* isr)();
    void set_clock(uint32_t frequency);
    void abort_transaction_async();
    bool start(uint8_t address, uint32_t direction);
    uint8_t tx_fifo_count();
    uint8_t rx_fifo_count();
    void clear_all_msr_flags();
};

extern IMX_RT1060_I2CMaster Master;     // Pins 19 and 18; SCL0 and SDA0
extern IMX_RT1060_I2CMaster Master1;    // Pins 16 and 17; SCL1 and SDA1
extern IMX_RT1060_I2CMaster Master2;    // Pins 24 and 25; SCL2 and SDA2

class IMX_RT1060_I2CSlave : public I2CSlave
{
public:
    IMX_RT1060_I2CSlave(IMXRT_LPI2C_Registers* port, IMX_RT1060_I2CBase::Config& config, void (* isr)())
            : port(port), config(config), isr(isr) {
    }

    void listen(uint8_t address) override;

    void listen(uint8_t first_address, uint8_t second_address) override;

    void listen_range(uint8_t first_address, uint8_t last_address) override;

    void stop_listening() override;

    // Call this to reset the slave when it ignores a new I2C transaction
    // because it's stuck waiting for the master to acknowledge a transmit.
    // (SBF flag is high and no interrupts are triggered.)
    void reset() {
        // Clear the slave's buffers and reset the internal state
        port->SCR = (LPI2C_SCR_RRF | LPI2C_SCR_RTF);
        state = State::idle;
        _error = I2CError::ok;
        port->SCR = LPI2C_SCR_SEN;
    }

    void after_receive(std::function<void(size_t length, uint16_t address)> callback) override;

    void before_transmit(std::function<void(uint16_t address)> callback) override;

    void after_transmit(std::function<void(uint16_t address)> callback) override;

    void set_transmit_buffer(uint8_t* buffer, size_t size) override;

    void set_receive_buffer(uint8_t* buffer, size_t size) override;

    void _interrupt_service_routine();

private:
    enum class State {
        // Busy states
        receiving = 0,  // Receiving bytes from the master
        transmitting,   // Transmitting bytes to the master

        // 'idle' and above mean that the driver has finished
        // whatever it was doing and is ready to do more work.
        idle = 100,     // Not in a transaction
        aborted,        // Transaction was aborted due to an error.
    };

    IMXRT_LPI2C_Registers* const port;
    IMX_RT1060_I2CBase::Config& config;
    volatile State state = State::idle;
    volatile uint16_t address_called = 0;

    I2CBuffer rx_buffer = I2CBuffer();
    I2CBuffer tx_buffer = I2CBuffer();
    bool trailing_byte_sent = false;

    void (* isr)();
    std::function<void(size_t length, uint16_t address)> after_receive_callback = nullptr;
    std::function<void(uint16_t address)> before_transmit_callback = nullptr;
    std::function<void(uint16_t address)> after_transmit_callback = nullptr;

    void listen(uint32_t samr, uint32_t address_config);

    // Called from within the ISR when we receive a Repeated START or STOP
    void end_of_frame();
};

extern IMX_RT1060_I2CSlave Slave;   // Pins 19 and 18; SCL0 and SDA0
extern IMX_RT1060_I2CSlave Slave1;  // Pins 16 and 17; SCL1 and SDA1
extern IMX_RT1060_I2CSlave Slave2;  // Pins 24 and 25; SCL2 and SDA2

#endif //IMX_RT1060_I2C_DRIVER_H
