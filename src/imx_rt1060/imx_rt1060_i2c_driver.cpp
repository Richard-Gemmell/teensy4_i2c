// Copyright © 2019-2023 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)
//
// Fragments of this code copied from WireIMXRT.cpp © Paul Stoffregen.
// Please support the Teensy project at pjrc.com.

//#define DEBUG_I2C // Uncomment to enable debug tools
#ifdef DEBUG_I2C
#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#endif

#include <Arduino.h>
#include <imxrt.h>
#include <pins_arduino.h>
#include "imx_rt1060_i2c_driver.h"

#define DUMMY_BYTE 0x00 // Used when there's no real data to write.
#define NUM_FIFOS 4     // Number of Rx and Tx FIFOs available to master
#define MASTER_READ 1   // Makes the address a read request
#define MASTER_WRITE 0  // Makes the address a write request
#define MAX_MASTER_READ_LENGTH 256  // Maximum number of bytes that can be read by a single Master read
#define CLOCK_STRETCH_TIMEOUT 15000 // Timeout if a device stretches SCL this long, in microseconds

// Debug tools
#ifdef DEBUG_I2C
static void log_slave_status_register(uint32_t ssr);
static void log_master_status_register(uint32_t msr);
static void log_master_control_register(const char* message, uint32_t mcr);
#endif
static void log_master_status_register(uint32_t msr);

static uint8_t empty_buffer[0];

I2CBuffer::I2CBuffer() : buffer(empty_buffer) {
}

struct I2CMasterConfiguration {
    uint8_t PRESCALE;
    uint8_t CLKHI;
    uint8_t CLKLO;
    uint8_t DATAVD;
    uint8_t SETHOLD;
    uint8_t FILTSDA;
    uint8_t FILTSCL;
    uint8_t BUSIDLE;
    uint16_t PINLOW;
};

struct I2CSlaveConfiguration {
    uint8_t DATAVD;
    uint8_t FILTSDA;
    uint8_t FILTSCL;
    uint8_t CLKHOLD;
};

//#define USE_OLD_CONFIG
#ifdef USE_OLD_CONFIG
const I2CMasterConfiguration DefaultStandardModeMasterConfiguration = {
    .PRESCALE = 1,
    .CLKHI = 55, .CLKLO = 59,
    .DATAVD = 25, .SETHOLD = 63,
    .FILTSDA = 5, .FILTSCL = 5,
    .BUSIDLE = 6, .PINLOW = CLOCK_STRETCH_TIMEOUT * 12 / 256 + 1
};

const I2CMasterConfiguration DefaultFastModeMasterConfiguration = {
    .PRESCALE = 0,
    .CLKHI = 26, .CLKLO = 28,
    .DATAVD = 12, .SETHOLD = 25,
    .FILTSDA = 2, .FILTSCL = 2,
    .BUSIDLE = 3, .PINLOW = CLOCK_STRETCH_TIMEOUT * 24 / 256 + 1
};

const I2CMasterConfiguration DefaultFastModePlusMasterConfiguration = {
    .PRESCALE = 0,
    .CLKHI = 9, .CLKLO = 10,
    .DATAVD = 4, .SETHOLD = 10,
    .FILTSDA = 1, .FILTSCL = 1,
    .BUSIDLE = 1, .PINLOW = CLOCK_STRETCH_TIMEOUT * 24 / 256 + 1
};

const I2CSlaveConfiguration DefaultSlaveConfiguration = {
    .DATAVD = 0, .FILTSDA = 0, .FILTSCL = 0, .CLKHOLD = 0
};
#else
const I2CMasterConfiguration DefaultStandardModeMasterConfiguration = {
    .PRESCALE = 3,
//    .CLKHI = 34, .CLKLO = 37,
    .CLKHI = 34, .CLKLO = 42,		// Fix for AIF06 device ack timer
    .DATAVD = 7, .SETHOLD = 44,
    .FILTSDA = 15, .FILTSCL = 15,
    .BUSIDLE = 9, .PINLOW = CLOCK_STRETCH_TIMEOUT * 15 / (2 * 256) + 1
};

const I2CMasterConfiguration DefaultFastModeMasterConfiguration = {
    .PRESCALE = 1,
    .CLKHI = 23, .CLKLO = 42,
    .DATAVD = 11, .SETHOLD = 21,
    .FILTSDA = 15, .FILTSCL = 15,
    .BUSIDLE = 1, .PINLOW = CLOCK_STRETCH_TIMEOUT * 30 / 256 + 1
};

const I2CMasterConfiguration DefaultFastModePlusMasterConfiguration = {
    .PRESCALE = 0,
    .CLKHI = 14, .CLKLO = 36,
    .DATAVD = 12, .SETHOLD = 19,
    .FILTSDA = 6, .FILTSCL = 6,
    .BUSIDLE = 1, .PINLOW = CLOCK_STRETCH_TIMEOUT * 60 / 256 + 1
};

const I2CSlaveConfiguration DefaultSlaveConfiguration = {
  .DATAVD = 3, .FILTSDA = 6, .FILTSCL = 6, .CLKHOLD = 0
};
#endif

// Slave Receiver NACKs
//
// The I2C Specification in section "3.1.6 Acknowledge (ACK) and Not Acknowledge (NACK)"
// says a receiver may send NACK if "the receiver cannot receive any more data bytes".
// The i.MX RT1062 I2C hardware automatically ACKs all received bytes in slave mode
// by default. This means that the slave appears to have an infinitely large buffer
// from the master's point of view. This mirrors the slave behaviour for transmitting
// where the slave _must_ provide bytes as long as the master demands them.
// I suspect that it's fairly common for slave devices to act this way. (RJG Feb 2023)
//
// It is possible to handle ACK and NACK in the driver software. See the datasheet
// documentation for TXNACK and SCFGR1[ACKSTALL].

// Pad Configuration Values
//
// NXP documents the pad configuration in AN5078.pdf Rev 0.
// https://www.nxp.com/docs/en/application-note/AN5078.pdf
//
// Enable Open Drain - See AN5078.pdf sections 5.2
// Required for I2C to avoid shorts
// Required Value: IOMUXC_PAD_ODE  // Enabled
//
// DSE - (Drive Strength Enable) - See AN5078.pdf section 7.1
// Used to tune the output driver's impedance to match the load impedance.
// If the impedance is too high the circuit behaves like a low pass filter,
// rounding off the inputs.
// Suggested Value: IOMUXC_PAD_DSE(2) - minimises undershoot spikes and change in Vol
//
// SPEED - See AN5078.pdf section 7.3
// "These options can either increase the output driver current in the higher
// frequency range, or reduce the switching noise in the lower frequency range."
// Suggested Value: IOMUXC_PAD_SPEED(0) // 50MHz
//
// Hysteresis - See AN5078.pdf section 7.2
// Reduces the number of false input signal changes (glitches) caused by noise
// near the centre point of the voltage range. Enabling this option "slightly
// increases the pin power consumption as well as the propagation delay by
// several nanoseconds." (From AN5078)
// Suggested Value: IOMUXC_PAD_HYS  // Enabled
//
// SRE - Slew Rate Field - See AN5078.pdf section 7.2
// NXP recommend that this option should be disabled for I2C
// Suggested Value: disabled. Add IOMUXC_PAD_SRE to enable
//
#define PAD_CONTROL_CONFIG (IOMUXC_PAD_ODE | IOMUXC_PAD_DSE(2) | IOMUXC_PAD_SPEED(0) | IOMUXC_PAD_HYS)

I2CDriver::I2CDriver()
    : pad_control_config(PAD_CONTROL_CONFIG), pullup_config(InternalPullup::enabled_22k_ohm) {
}

static void initialise_pin(IMX_RT1060_I2CBase::PinInfo pin, uint32_t pad_control_config, InternalPullup pullup) {
    uint32_t pullup_config = 0x0;
    if(pullup != InternalPullup::disabled) {
        pullup_config = IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(static_cast<uint32_t>(pullup));
    }

    const uint32_t pullup_mask = 0b1'00001000'11111001;
    *(portControlRegister(pin.pin)) = (pad_control_config & pullup_mask) | pullup_config;
    #ifdef DEBUG_I2C
    Serial.print("Pad control register: 0x");
    Serial.println(*(portControlRegister(pin.pin)), 16);
    #endif

    *(portConfigRegister(pin.pin)) = pin.mux_val;
    if (pin.select_input_register) {
        *(pin.select_input_register) = pin.select_val;
    }
}

void initialise_common(IMX_RT1060_I2CBase::Config hardware, uint32_t pad_control_config, InternalPullup pullup) {
#ifdef USE_OLD_CONFIG
    // Set LPI2C Clock to 24 MHz. Required by slaves as well as masters.
    CCM_CSCDR2 = (CCM_CSCDR2 & ~CCM_CSCDR2_LPI2C_CLK_PODF(63)) | CCM_CSCDR2_LPI2C_CLK_SEL;
#else
    // Set LPI2C Clock to 60 MHz. Required by slaves as well as masters.
    CCM_CSCDR2 = (CCM_CSCDR2 & ~CCM_CSCDR2_LPI2C_CLK_PODF(63));
#endif
    hardware.clock_gate_register |= hardware.clock_gate_mask;

    // Setup SDA and SCL pins and registers
    initialise_pin(hardware.sda_pin, pad_control_config, pullup);
    initialise_pin(hardware.scl_pin, pad_control_config, pullup);
}

static void stop(IMXRT_LPI2C_Registers* port, IRQ_NUMBER_t irq) {
    // Halt and reset Master Mode if it's running
    port->MCR = (LPI2C_MCR_RST | LPI2C_MCR_RRF | LPI2C_MCR_RTF);
    port->MCR = 0;

    // Halt and reset Slave Mode if it's running
    port->SCR = (LPI2C_SCR_RST | LPI2C_SCR_RRF | LPI2C_SCR_RTF);
    port->SCR = 0;

    // Disable interrupts
    NVIC_DISABLE_IRQ(irq);
    attachInterruptVector(irq, nullptr);
}

IMX_RT1060_I2CMaster::IMX_RT1060_I2CMaster(IMXRT_LPI2C_Registers* port, IMX_RT1060_I2CBase::Config& config, void (* isr)())
        : port(port), config(config), isr(isr) {
}

void IMX_RT1060_I2CMaster::begin(uint32_t frequency) {
    // Make sure master mode is disabled before configuring it.
    stop(port, config.irq);

    // Setup pins and master clock
    initialise_common(config, pad_control_config, pullup_config);

	// Set our state as idle
	state = State::idle;

    // Configure and Enable Master Mode
    // Set FIFO watermarks. Determines when the RDF and TDF interrupts happen
    port->MFCR = LPI2C_MFCR_RXWATER(0) | LPI2C_MFCR_TXWATER(0);
    set_clock(frequency);

    // Setup interrupt service routine.
   attachInterruptVector(config.irq, isr);
   port->MIER = LPI2C_MIER_RDIE | LPI2C_MIER_SDIE | LPI2C_MIER_NDIE | LPI2C_MIER_ALIE | LPI2C_MIER_FEIE | LPI2C_MIER_PLTIE;
   NVIC_ENABLE_IRQ(config.irq);
}

void IMX_RT1060_I2CMaster::end() {
    stop(port, config.irq);
}

inline bool IMX_RT1060_I2CMaster::finished() {
    return state == State::transfer_complete ||
        (state >= State::idle && !(port->MSR & LPI2C_MSR_MBF));
}

size_t IMX_RT1060_I2CMaster::get_bytes_transferred() {
    return buff.get_bytes_transferred();
}

void IMX_RT1060_I2CMaster::write_async(uint8_t address, const uint8_t* buffer, size_t num_bytes, bool send_stop) {
    #ifdef DEBUG_I2C
    Serial.printf("%s: %lu Write_asynch addr:0x%02x  #Bytes: %d,  Stop: %d, tx_fifo: %d\n\r",__func__,micros(), address, num_bytes, send_stop, tx_fifo_count());
    log_master_status_register(port->MSR);
    #endif

    if (!start(address, MASTER_WRITE)) {
        return;
    }
    if (num_bytes == 0) {
        // The caller is probably probing addresses to find slaves.
        // Don't try to transmit anything.
        ignore_tdf = true;
        port->MTDR = LPI2C_MTDR_CMD_STOP;
        return;
    }

    buff.initialise(const_cast<uint8_t*>(buffer), num_bytes);
    stop_on_completion = send_stop;
    port->MIER |= LPI2C_MIER_TDIE;
}

void IMX_RT1060_I2CMaster::read_async(uint8_t address, uint8_t* buffer, size_t num_bytes, bool send_stop) {
    if (num_bytes > MAX_MASTER_READ_LENGTH) {
        _error = I2CError::invalid_request;
        return;
    }

    #ifdef DEBUG_I2C
    Serial.printf("%s: %lu Read_asynch addr:0x%02x  #Bytes: %d,  Stop: %d, tx_fifo: %d\n\r",__func__,micros(), address, num_bytes, send_stop, tx_fifo_count());
    log_master_status_register(port->MSR);
    #endif
    if (!start(address, MASTER_READ)) {
        return;
    }
	
    if (num_bytes == 0) {
        // The caller is probably probing addresses to find slaves.
        // Don't try to read anything.
        port->MTDR = LPI2C_MTDR_CMD_STOP;
        return;
    }

    #ifdef DEBUG_I2C
    Serial.printf("%s: %lu setting the read regs tx_fifo: %d\n\r",__func__,micros(), tx_fifo_count());
    #endif

    buff.initialise(buffer, num_bytes);
    port->MTDR = LPI2C_MTDR_CMD_RECEIVE | (num_bytes - 1);

    if (send_stop) {
        port->MTDR = LPI2C_MTDR_CMD_STOP;
    }
}

// Do not call this method directly
void IMX_RT1060_I2CMaster::_interrupt_service_routine() {
    uint32_t msr = port->MSR;
    uint32_t mcr = port->MCR;
    uint32_t mcfgr0 = port->MCFGR0;	
    uint32_t mcfgr1 = port->MCFGR1;	
    uint32_t mcfgr2 = port->MCFGR2;	
    uint32_t mcfgr3 = port->MCFGR3;	
	
    #ifdef DEBUG_I2C
    Serial.printf("ISR: %lu enter: msr:0x%08x  mcr: 0x%08x   mcfgr0: 0x%08x   mcfgr1: 0x%08x   mcfgr2: 0x%08x   mcfgr3: 0x%08x   \n\r",micros(), msr, mcr, mcfgr0, mcfgr1, mcfgr2, mcfgr3);
    log_master_status_register(msr);
    #endif

    if (msr & (LPI2C_MSR_NDF | LPI2C_MSR_ALF | LPI2C_MSR_FEF | LPI2C_MSR_PLTF)) {
        if (msr & LPI2C_MSR_NDF) {
            port->MSR = LPI2C_MSR_NDF;
			// This test not perfect (ideally we would detect only the first byte not transmitted
            if ((state == State::starting) || ((state == (State::transferring)) && (tx_fifo_count()>0)))  {
                _error = I2CError::address_nak;
				// Don't handle anymore TDF interrupts
				port->MIER &= ~LPI2C_MIER_TDIE;

				// Clear out any commands that haven't been sent
				port->MCR |= LPI2C_MCR_RTF;
				port->MCR |= LPI2C_MCR_RRF;

				// Send a stop if haven't already done so and still control the bus
				if ((msr & LPI2C_MSR_MBF) && !(msr & LPI2C_MSR_SDF)) {
					port->MTDR = LPI2C_MTDR_CMD_STOP;
				}
				#ifdef DEBUG_I2C
				Serial.printf("Master: %lu Address NAK\n\r",micros());
				#endif
				return;
            } else {
				#ifdef DEBUG_I2C
				Serial.printf("Master: %lu Data NAK\n\r", micros());
				#endif
                _error = I2CError::data_nak;
            }
			// End the transaction
			abort_transaction_async();
			return;
        }
        if (msr & LPI2C_MSR_ALF) {
            port->MSR = LPI2C_MSR_ALF;
            _error = I2CError::arbitration_lost;
            #ifdef DEBUG_I2C
            Serial.printf("Master: %lu Arbitration lost\n\r",micros());
            #endif
        }
        if (msr & LPI2C_MSR_FEF) {
            port->MSR = LPI2C_MSR_FEF | LPI2C_MSR_EPF;
            if (!has_error()) {
				#ifdef DEBUG_I2C
				Serial.printf("Master: %lu Fifo error", micros());
				#endif
                _error = I2CError::master_fifo_error;
            }
            // else FEF was triggered by another error. Ignore it.
			#ifdef DEBUG_I2C
			else Serial.printf("Master: %lu other error: msr:0x%08x\n\r",micros(), msr);
			#endif
        }
        if (msr & LPI2C_MSR_PLTF) {
            #ifdef DEBUG_I2C
            Serial.printf("Master: %lu Pin low timeout (PLTF)\n\r", micros());
            #endif
 //           port->MSR = LPI2C_MSR_PLTF;
            _error = I2CError::master_pin_low_timeout;
            state = State::stopping;
            abort_transaction_async();
        }
        if (state != State::stopping) {
            #ifdef DEBUG_I2C
            Serial.printf("Master: %lu forced stopping (PLTF)\n\r",micros());
            #endif
            state = State::stopping;
            abort_transaction_async();
        }
    }

    if (msr & LPI2C_MSR_SDF) {
        #ifdef DEBUG_I2C
        Serial.printf("Master: %lu Stopping for SDF\n\r",micros());
        #endif

        port->MIER &= ~LPI2C_MIER_TDIE; // We don't want to handle TDF if we can avoid it.
        state = State::stopped;
        port->MSR = LPI2C_MSR_SDF;
    }

    if (msr & LPI2C_MSR_RDF) {
        #ifdef DEBUG_I2C
		uint32_t mfsr = port->MFSR;
        Serial.printf("Master: %lu RDF  TX: %u  RX: %u\n\r",micros(),mfsr & 0xff, (mfsr >>16) & 0xff);
        #endif
        if (ignore_tdf) {
            if (buff.not_started_reading()) {
                _error = I2CError::ok;
                state = State::transferring;
            }
            if (state == State::transferring) {
                buff.write(port->MRDR);
            } else {
                port->MCR |= LPI2C_MCR_RRF;
            }
            if (buff.finished_reading()) {
				#ifdef DEBUG_I2C
				Serial.printf("Master: %lu finished reading tx_fifo: %d rx_fifo: %d\n\r",micros(),tx_fifo_count(),rx_fifo_count());
				#endif
				// Flush RX fifo if we have everything we are looking for
				while (rx_fifo_count() > 0) {
					port->MRDR;
				}
                if (tx_fifo_count() == 1) {
                    state = State::stopping;
                } else {
                    state = State::transfer_complete;
                }
                port->MCR &= ~LPI2C_MCR_MEN;    // Avoids triggering PLTF if we didn't send a STOP
            }
        } else {
            // This is a write transaction. We shouldn't have got a read.
			#ifdef DEBUG_I2C
			Serial.printf("Master: %lu Unexpected read during a write\n\r",micros());
			#endif
            state = State::stopping;
            abort_transaction_async();
        }
    }

    if (!ignore_tdf && (msr & LPI2C_MSR_TDF)) {
		#ifdef DEBUG_I2C
		Serial.printf("Master: %lu Not ignore TDF & TDF\n\r", micros());
		#endif
        if (buff.not_started_writing()) {
            _error = I2CError::ok;
            state = State::transferring;
        }
        if (state == State::transferring) {
            // Fill the transmit buffer
            uint32_t fifo_space = NUM_FIFOS - tx_fifo_count();
            while (buff.has_data_available() && fifo_space > 0) {
                port->MTDR = LPI2C_MTDR_CMD_TRANSMIT | buff.read();
                fifo_space--;
            }
            if (buff.finished_writing() && tx_fifo_count() == 0) {
                port->MIER &= ~LPI2C_MIER_TDIE;
                if (stop_on_completion) {
                    state = State::stopping;
                    port->MTDR = LPI2C_MTDR_CMD_STOP;
					#ifdef DEBUG_I2C
					Serial.printf("Master: %lu write complete, stopping\n\r", micros());
					#endif
                } else {
					#ifdef DEBUG_I2C
					Serial.printf("Master: %lu Write complete, setting transfer_complete\n\r", micros());
					#endif
                    state = State::transfer_complete;
                }
                port->MCR &= ~LPI2C_MCR_MEN;    // Avoids triggering PLTF if we didn't send a STOP
            }
        }
        // else ignore it. This flag is frequently set in read transfers.
    }
}

inline uint8_t IMX_RT1060_I2CMaster::tx_fifo_count() {
    return port->MFSR & 0x7;
}

inline uint8_t IMX_RT1060_I2CMaster::rx_fifo_count() {
    return (port->MFSR >> 16) & 0x07;
}

inline void IMX_RT1060_I2CMaster::clear_all_msr_flags() {
    port->MSR &= (LPI2C_MSR_DMF | LPI2C_MSR_PLTF | LPI2C_MSR_FEF |
                  LPI2C_MSR_ALF | LPI2C_MSR_NDF | LPI2C_MSR_SDF |
                  LPI2C_MSR_EPF | LPI2C_MSR_RDF | LPI2C_MSR_TDF);
}


void IMX_RT1060_I2CMaster::reset() {

	#ifdef DEBUG_I2C
	Serial.printf("Master: %lu Doing Reset\n\r", micros());
	#endif

	// Stop the normal I2C processing
	stop(port,config.irq);
	
	// Now bit-bang the bus until we get our signals back
	uint32_t sda_pin = config.sda_pin.pin;
	uint32_t scl_pin = config.scl_pin.pin;
	uint32_t sda_mask = digitalPinToBitMask(sda_pin);
	uint32_t scl_mask = digitalPinToBitMask(scl_pin);

	*portConfigRegister(scl_pin) = 5 | 0x10;
	*portSetRegister(scl_pin) = scl_mask;
	*portModeRegister(scl_pin) |= scl_mask;
	*portConfigRegister(sda_pin) = 5 | 0x10;
	*portSetRegister(sda_pin) = sda_mask;
	*portModeRegister(sda_pin) |= sda_mask;

	// Now do 9 simulated clock cycles (@ 100Khz) to clear everything out
	delayMicroseconds(10);
	for (int i=0; i < 9; i++) {
		if ((*portInputRegister(sda_pin) & sda_mask)
		  && (*portInputRegister(scl_pin) & scl_mask)) {
			break;
		}
		*portClearRegister(scl_pin) = scl_mask;
		delayMicroseconds(5);
		*portSetRegister(scl_pin) = scl_mask;
		delayMicroseconds(5);
	}

	// And restart the normal processing
	begin(savedFrequency);
}


bool IMX_RT1060_I2CMaster::start(uint8_t address, uint32_t direction) {
    if (!finished()) {
        // We haven't completed the previous transaction yet
        #ifdef DEBUG_I2C
        Serial.print("Master: Cannot start. Transaction still in progress. State: ");
        Serial.print((int)state);
        Serial.print(". Error code: ");
        Serial.println((int)_error);
        #endif

        abort_transaction_async();

        _error = I2CError::master_not_ready;
        state = State::idle;
        return false;
    }

    // Start a new transaction
    ignore_tdf = direction;
    _error = I2CError::ok;
    state = State::starting;

    // Make sure the FIFOs are empty before we start.
    if (tx_fifo_count() > 0 || rx_fifo_count() > 0) {
        // This should never happen.
        #ifdef DEBUG_I2C
        Serial.print("Master: FIFOs not empty in start(). TX: ");
        Serial.print(tx_fifo_count());
        Serial.print(" RX: ");
        Serial.println(rx_fifo_count());
        #endif
        _error = I2CError::master_fifos_not_empty;
        abort_transaction_async();
        return false;
    }

    // Clear status flags
    clear_all_msr_flags();

    // Send a START to the slave at 'address'
    port->MCR |= LPI2C_MCR_MEN;
    uint8_t i2c_address = (address & 0x7F) << 1;
    port->MTDR = LPI2C_MTDR_CMD_START | i2c_address | direction;

    return true;
}

// In theory, you can use MCR[RST] to reset the master but
// this doesn't seem to work in some circumstances. e.g.
// When the master is trying to receive more bytes.
void IMX_RT1060_I2CMaster::abort_transaction_async() {
    #ifdef DEBUG_I2C
    Serial.println("Master: abort_transaction");
    log_master_status_register(port->MSR);
    #endif

    // Don't handle anymore TDF interrupts
    port->MIER &= ~LPI2C_MIER_TDIE;
//    port->MCR &= ~LPI2C_MCR_MEN;    // Avoids triggering PLTF if we didn't send a STOP

    // Clear out any commands that haven't been sent
    port->MCR |= LPI2C_MCR_RTF;
    port->MCR |= LPI2C_MCR_RRF;

    // Send a stop if haven't already done so and still control the bus
    uint32_t msr = port->MSR;
	if (msr & LPI2C_MSR_PLTF) {
		// Lost arbitration, need to do a reset
		Serial.printf("%s - %ul Lost bus, msr: 0x%08x\n\r",__func__,millis(), msr);
        port->MTDR = LPI2C_MTDR_CMD_STOP;
		reset();
	}
    else if ((msr & LPI2C_MSR_MBF) && !(msr & LPI2C_MSR_SDF)) {
        #ifdef DEBUG_I2C
        Serial.println("  sending STOP");
        #endif
//        port->MTDR = LPI2C_MTDR_CMD_STOP;
		// One more time Clear out any commands that haven't been sent
		port->MCR |= LPI2C_MCR_RTF;
		port->MCR |= LPI2C_MCR_RRF;
        Serial.println("  ABRT: sending STOP");
    }
	else {
		
        port->MTDR = LPI2C_MTDR_CMD_STOP;
        Serial.println("  ABRT: else sending STOP");
	}
}

// Supports 100 kHz, 400 kHz and 1 MHz modes.
void IMX_RT1060_I2CMaster::set_clock(uint32_t frequency) {
    I2CMasterConfiguration timings;
	savedFrequency = frequency;
    if (frequency < 400'000) {
        // Use Standard Mode - up to 100 kHz
        timings = DefaultStandardModeMasterConfiguration;
    } else if (frequency < 1'000'000) {
        // Use Fast Mode - up to 400 kHz
        timings = DefaultFastModeMasterConfiguration;
    } else {
        // Use Fast Mode Plus - up to 1 MHz
        timings = DefaultFastModePlusMasterConfiguration;
    }
    port->MCCR0 = LPI2C_MCCR0_CLKHI(timings.CLKHI) | LPI2C_MCCR0_CLKLO(timings.CLKLO) |
                  LPI2C_MCCR0_DATAVD(timings.DATAVD) | LPI2C_MCCR0_SETHOLD(timings.SETHOLD);
    port->MCFGR1 = LPI2C_MCFGR1_PRESCALE(timings.PRESCALE);
    port->MCFGR2 = LPI2C_MCFGR2_FILTSDA(timings.FILTSDA) | LPI2C_MCFGR2_FILTSCL(timings.FILTSCL) |
                   LPI2C_MCFGR2_BUSIDLE(timings.BUSIDLE);
    port->MCFGR3 = LPI2C_MCFGR3_PINLOW(timings.PINLOW);
    port->MCCR1 = port->MCCR0;
}

void IMX_RT1060_I2CSlave::listen(uint8_t address) {
    // Listen to a single 7-bit address
    uint32_t samr = LPI2C_SAMR_ADDR0(address);
    uint32_t address_config = LPI2C_SCFGR1_ADDRCFG(0x0);
    listen(samr, address_config);
}

void IMX_RT1060_I2CSlave::listen(uint8_t first_address, uint8_t second_address) {
    // Listen to 2 7-bit addresses
    uint32_t samr = LPI2C_SAMR_ADDR0(first_address) | LPI2C_SAMR_ADDR1(second_address);
    uint32_t address_config = LPI2C_SCFGR1_ADDRCFG(0x02);
    listen(samr, address_config);
}

void IMX_RT1060_I2CSlave::listen_range(uint8_t first_address, uint8_t last_address) {
    // Listen to all 7-bit addresses in the range (inclusive)
    uint32_t samr = LPI2C_SAMR_ADDR0(first_address) | LPI2C_SAMR_ADDR1(last_address);
    uint32_t address_config = LPI2C_SCFGR1_ADDRCFG(0x06);
    listen(samr, address_config);
}

void IMX_RT1060_I2CSlave::listen(uint32_t samr, uint32_t address_config) {
    // Make sure slave mode is disabled before configuring it.
    stop_listening();

    initialise_common(config, pad_control_config, pullup_config);

    // Clear previous state
    _error = I2CError::ok;

    // Set the Slave Address
    port->SAMR = samr;

    // Use the same timings for all modes
    const I2CSlaveConfiguration timings = DefaultSlaveConfiguration;

    // Data Valid Time. Determines how long slave waits before changing
    // SDA value after the previous clock pulse in preparation for the next clock.
    // Affects slave ACK, NACK and data bits. (tVD:DAT, tVD;ACK in I2C Spec)
    // Requires SCR[FILTEN] to be set. Affected by SCR[FILTDZ]. Disabled if high speed mode is enabled.
    port->SCFGR2 = LPI2C_SCFGR2_DATAVD(timings.DATAVD);

    // Glitch Filter. Suppresses noise.
    // Rules for enabling/disabling are the same as for the data valid time. See above.
    // Note that FILTSDA must be >= FILTSCL
    // Seems to hang the Teensy if LPI2C_SCFGR2_FILTSDA is 5.
    port->SCFGR2 = port->SCFGR2 | LPI2C_SCFGR2_FILTSDA(timings.FILTSDA) | LPI2C_SCFGR2_FILTSCL(timings.FILTSCL);

    // Clock Hold Time. Sets the minimum clock hold time when clock stretching
    port->SCFGR2 = port->SCFGR2 | LPI2C_SCFGR2_CLKHOLD(timings.CLKHOLD);

    // Enable clock stretching and set address
    port->SCFGR1 = (address_config | LPI2C_SCFGR1_TXDSTALL | LPI2C_SCFGR1_RXSTALL);

    // Set up interrupts
    attachInterruptVector(config.irq, isr);
    port->SIER = (LPI2C_SIER_RSIE | LPI2C_SIER_SDIE | LPI2C_SIER_TDIE | LPI2C_SIER_RDIE);
    NVIC_ENABLE_IRQ(config.irq);

    // Enable Slave Mode
    port->SCR = LPI2C_SCR_SEN | LPI2C_SCR_FILTEN;
}

inline void IMX_RT1060_I2CSlave::stop_listening() {
    // End slave mode
    stop(port, config.irq);
}

inline void IMX_RT1060_I2CSlave::after_receive(std::function<void(size_t len, uint16_t address)> callback) {
    after_receive_callback = callback;
}

inline void IMX_RT1060_I2CSlave::before_transmit(std::function<void(uint16_t address)> callback) {
    before_transmit_callback = callback;
}

inline void IMX_RT1060_I2CSlave::after_transmit(std::function<void(uint16_t address)> callback) {
    after_transmit_callback = callback;
}

inline void IMX_RT1060_I2CSlave::set_transmit_buffer(const uint8_t* buffer, size_t size) {
    tx_buffer.initialise(const_cast<uint8_t*>(buffer), size);
}

inline void IMX_RT1060_I2CSlave::set_receive_buffer(uint8_t* buffer, size_t size) {
    rx_buffer.initialise(buffer, size);
}

// WARNING: Do not call directly.
void IMX_RT1060_I2CSlave::_interrupt_service_routine() {
    // Read the slave status register
    uint32_t ssr = port->SSR;
//    log_slave_status_register(ssr);

    if (ssr & LPI2C_SSR_AVF) {
        // Find out which address was used and clear to the address flag.
        address_called = (port->SASR & LPI2C_SASR_RADDR(0x7FF)) >> 1;
    }

    if (ssr & (LPI2C_SSR_RSF | LPI2C_SSR_SDF)) {
        // Detected Repeated START or STOP
        port->SSR = (LPI2C_SSR_RSF | LPI2C_SSR_SDF);
        end_of_frame();
    }

    if (ssr & LPI2C_SSR_RDF) {
        //  Received Data
        uint32_t srdr = port->SRDR; // Read the Slave Received Data Register
        if (srdr & LPI2C_SRDR_SOF) {
            // Start of Frame (The first byte since a (repeated) START or STOP condition)
            _error = I2CError::ok;
            if (rx_buffer.initialised()) {
                rx_buffer.reset();
                state = State::receiving;
            }
        }
        uint8_t data = srdr & LPI2C_SRDR_DATA(0xFF);
        if (rx_buffer.initialised()) {
            if (!rx_buffer.write(data)) {
                // The buffer is already full.
                // Don't NACK. Just swallow the byte. See "Slave Receiver NACKs" above.
                _error = I2CError::buffer_overflow;
            }
        } else {
            // We are not interested in reading anything.
            // Don't NACK. Just swallow the byte. See "Slave Receiver NACKs" above.
            _error = I2CError::buffer_overflow;
            state = State::idle;
        }
    }

    if (ssr & LPI2C_SSR_TDF) {
        // Transmit Data Request - Master is requesting a byte
        bool start_of_frame = state >= State::idle;
        if (start_of_frame) {
            _error = I2CError::ok;
            state = State::transmitting;
            if (before_transmit_callback) {
                before_transmit_callback(address_called);
            }
        }
        if (tx_buffer.initialised()) {
            if (start_of_frame) {
                tx_buffer.reset();
            }
            if (tx_buffer.has_data_available()) {
                port->STDR = tx_buffer.read();
            } else {
                port->STDR = DUMMY_BYTE;
                // WARNING: We're always asked for one more byte than
                // the master actually requested. Use trailing_byte_sent
                // to work out whether the master actually asked for too much data.
                if (!trailing_byte_sent) {
                    trailing_byte_sent = true;
                } else {
                    _error = I2CError::buffer_underflow;
                }
            }
        } else {
            // We don't have any data to send.
            // Just send a dummy value instead.
            port->STDR = DUMMY_BYTE;
            _error = I2CError::buffer_underflow;
        }
    }

    if (ssr & LPI2C_SSR_FEF) {
        port->SSR = LPI2C_SSR_FEF;
        // Will not happen if clock stretching is enabled.
    }

    if (ssr & LPI2C_SSR_BEF) {
        #ifdef DEBUG_I2C
        Serial.println("I2C Slave: Bit Error");
        #endif
        // The bus is probably stuck at this point.
        // I don't think the slave can clear the fault. The master has to do it.
        port->SSR = LPI2C_SSR_BEF;
        state = State::aborted;
        _error = I2CError::bit_error;
        end_of_frame();
    }
}

// Called from within the ISR when we receive a Repeated START or STOP
void IMX_RT1060_I2CSlave::end_of_frame() {
    if (state == State::receiving) {
        if (after_receive_callback) {
            after_receive_callback(rx_buffer.get_bytes_transferred(), address_called);
        }
    } else if (state == State::transmitting) {
        trailing_byte_sent = false;
        if (after_transmit_callback) {
            after_transmit_callback(address_called);
        }
    }
    #ifdef DEBUG_I2C
    else if (state != State::idle) {
        Serial.print("Unexpected 'End of Frame'. State: ");
        Serial.println((int)state);
    }
    if (_error == I2CError::bit_error) {
        Serial.println("Transaction aborted because of bit error.");
    }
    #endif
    state = State::idle;
}

IMX_RT1060_I2CBase::Config i2c1_config = {
        CCM_CCGR2,
        CCM_CCGR2_LPI2C1(CCM_CCGR_ON),
        IMX_RT1060_I2CBase::PinInfo{18U, 3U | 0x10U, &IOMUXC_LPI2C1_SDA_SELECT_INPUT, 1U},
        IMX_RT1060_I2CBase::PinInfo{19U, 3U | 0x10U, &IOMUXC_LPI2C1_SCL_SELECT_INPUT, 1U},
        false,
        {},
        {},
        IRQ_LPI2C1
};

IMX_RT1060_I2CBase::Config i2c3_config = {
        CCM_CCGR2,
        CCM_CCGR2_LPI2C3(CCM_CCGR_ON),
        IMX_RT1060_I2CBase::PinInfo{17U, 1U | 0x10U, &IOMUXC_LPI2C3_SDA_SELECT_INPUT, 2U},
        IMX_RT1060_I2CBase::PinInfo{16U, 1U | 0x10U, &IOMUXC_LPI2C3_SCL_SELECT_INPUT, 2U},
        true,
        IMX_RT1060_I2CBase::PinInfo{36U, 2U | 0x10U, &IOMUXC_LPI2C3_SDA_SELECT_INPUT, 1U},
        IMX_RT1060_I2CBase::PinInfo{37U, 2U | 0x10U, &IOMUXC_LPI2C3_SCL_SELECT_INPUT, 1U},
        IRQ_LPI2C3
};

IMX_RT1060_I2CBase::Config i2c4_config = {
        CCM_CCGR6,
        CCM_CCGR6_LPI2C4_SERIAL(CCM_CCGR_ON),
        IMX_RT1060_I2CBase::PinInfo{25U, 0U | 0x10U, &IOMUXC_LPI2C4_SDA_SELECT_INPUT, 1U},
        IMX_RT1060_I2CBase::PinInfo{24U, 0U | 0x10U, &IOMUXC_LPI2C4_SCL_SELECT_INPUT, 1U},
        false,
        {},
        {},
        IRQ_LPI2C4
};

static void master_isr();

IMX_RT1060_I2CMaster Master(&LPI2C1, i2c1_config, master_isr);

static void master_isr() {
    Master._interrupt_service_routine();
}

static void master1_isr();

IMX_RT1060_I2CMaster Master1(&LPI2C3, i2c3_config, master1_isr);

static void master1_isr() {
    Master1._interrupt_service_routine();
}

static void master2_isr();

IMX_RT1060_I2CMaster Master2(&LPI2C4, i2c4_config, master2_isr);

static void master2_isr() {
    Master2._interrupt_service_routine();
}

static void slave_isr();

IMX_RT1060_I2CSlave Slave(&LPI2C1, i2c1_config, slave_isr);

static void slave_isr() {
    Slave._interrupt_service_routine();
}

static void slave1_isr();

IMX_RT1060_I2CSlave Slave1(&LPI2C3, i2c3_config, slave1_isr);

static void slave1_isr() {
    Slave1._interrupt_service_routine();
}

static void slave2_isr();

IMX_RT1060_I2CSlave Slave2(&LPI2C4, i2c4_config, slave2_isr);

static void slave2_isr() {
    Slave2._interrupt_service_routine();
}

#ifdef DEBUG_I2C
static void log_master_control_register(const char* message, uint32_t mcr) {
    Serial.print(message);
    Serial.print(" MCR: ");
    Serial.print(mcr);
    Serial.println("");
}
#endif

#ifdef DEBUG_I2C
static void log_master_status_register(uint32_t msr) {
 //   return;
    if (msr) {
        Serial.print("MSR Flags: ");
    }
    if (msr & LPI2C_MSR_BBF) {
        Serial.print("BBF ");
    }
    if (msr & LPI2C_MSR_MBF) {
        Serial.print("MBF ");
    }
    if (msr & LPI2C_MSR_DMF) {
        Serial.print("DMF ");
    }
    if (msr & LPI2C_MSR_PLTF) {
        Serial.print("PLTF ");
    }
    if (msr & LPI2C_MSR_FEF) {
        Serial.print("FEF ");
    }
    if (msr & LPI2C_MSR_ALF) {
        Serial.print("ALF ");
    }
    if (msr & LPI2C_MSR_NDF) {
        Serial.print("NDF ");
    }
    if (msr & LPI2C_MSR_SDF) {
        Serial.print("SDF ");
    }
    if (msr & LPI2C_MSR_EPF) {
        Serial.print("EPF ");
    }
    if (msr & LPI2C_MSR_RDF) {
        Serial.print("RDF ");
    }
    if (msr & LPI2C_MSR_TDF) {
        Serial.print("TDF ");
    }
    if (msr) {
        Serial.println();
    }
}
#endif

#ifdef DEBUG_I2C
static void log_slave_status_register(uint32_t ssr) {
    if (ssr) {
        Serial.print("SSR Flags: ");
    }
    if (ssr & LPI2C_SSR_BBF) {
        Serial.print("BBF ");
    }
    if (ssr & LPI2C_SSR_SBF) {
        Serial.print("SBF ");
    }
    if (ssr & LPI2C_SSR_SARF) {
        Serial.print("SARF ");
    }
    if (ssr & LPI2C_SSR_GCF) {
        Serial.print("GCF ");
    }
    if (ssr & LPI2C_SSR_AM1F) {
        Serial.print("GCF ");
    }
    if (ssr & LPI2C_SSR_AM0F) {
        Serial.print("AM0F ");
    }
    if (ssr & LPI2C_SSR_FEF) {
        Serial.print("FEF ");
    }
    if (ssr & LPI2C_SSR_BEF) {
        Serial.print("BBF ");
    }
    if (ssr & LPI2C_SSR_SDF) {
        Serial.print("SDF ");
    }
    if (ssr & LPI2C_SSR_RSF) {
        Serial.print("RSF ");
    }
    if (ssr & LPI2C_SSR_TAF) {
        Serial.print("TAF ");
    }
    if (ssr & LPI2C_SSR_AVF) {
        Serial.print("AVF ");
    }
    if (ssr & LPI2C_SSR_RDF) {
        Serial.print("RDF ");
    }
    if (ssr & LPI2C_SSR_TDF) {
        Serial.print("TDF ");
    }
    if (ssr) {
        Serial.println();
    }
}
#pragma clang diagnostic pop
#endif  //DEBUG_I2C
