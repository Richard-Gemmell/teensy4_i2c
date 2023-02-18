// Copyright (c) 2023 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_SLAVE_LOGIC_H
#define TEENSY4_I2C_TEST_E2E_SLAVE_LOGIC_H

#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"
#include "e2e/loopback/loopback_test_base.h"
#include <imx_rt1060/imx_rt1060_i2c_driver.h>
#include <bus_trace/bus_trace_builder.h>

namespace e2e {
namespace loopback {
namespace logic {

// Tests the logic of the driver in slave mode
class SlaveLogicTest : public LoopbackTestBase {
public:
    const static uint32_t FREQUENCY = 100'000;
    static I2CMaster& master;
    static I2CSlave& slave;

    void setUp() override {
        LoopbackTestBase::setUp();
        master.begin(FREQUENCY);
    }

    static void read_1_byte() {
        uint8_t rx_buffer = 0;
        master.read_async(ADDRESS, (uint8_t*)&rx_buffer, sizeof(rx_buffer), true);
        finish(master);
    }

    static void read_2_bytes() {
        uint16_t rx_buffer = 0;
        master.read_async(ADDRESS, (uint8_t*)&rx_buffer, sizeof(rx_buffer), true);
        finish(master);
    }

    static void master_reads_before_slave_sets_transmit_buffer() {
        // GIVEN the slave has not set a transmit buffer
        slave.listen(ADDRESS);

        // WHEN a master attempts to read from the slave
        uint16_t rx_buffer = 0;
        master.read_async(ADDRESS, (uint8_t*)&rx_buffer, sizeof(rx_buffer), true);
        finish(master);

        // THEN the master receives dummy bytes
        TEST_ASSERT_EQUAL(0x0000, rx_buffer);
        // AND the slave sets the error state
        TEST_ASSERT_TRUE(slave.has_error());
        TEST_ASSERT_EQUAL(I2CError::buffer_underflow, slave.error());
    }

    static void master_reads_too_many_bytes() {
        // GIVEN the slave is prepared to write some data
        slave.listen(ADDRESS);
        uint8_t tx_buffer = 0x17;
        slave.set_transmit_buffer((uint8_t*)&tx_buffer, sizeof(tx_buffer));

        // WHEN a master attempts to read more bytes than the slave has available
        uint32_t rx_buffer = 0;
        master.read_async(ADDRESS, (uint8_t*)&rx_buffer, sizeof(rx_buffer), true);
        finish(master);

        // THEN the master receives dummy bytes
        TEST_ASSERT_EQUAL(0x0017, rx_buffer);
        // AND the slave sets the error state
        TEST_ASSERT_TRUE(slave.has_error());
        TEST_ASSERT_EQUAL(I2CError::buffer_underflow, slave.error());
    }

    static void master_writes_before_slave_sets_receive_buffer() {
        // GIVEN the slave has not set a receive buffer
        slave.listen(ADDRESS);

        // WHEN a master attempts to write to the slave
        uint16_t tx_buffer = 0x17;
        master.write_async(ADDRESS, (uint8_t*)&tx_buffer, sizeof(tx_buffer), true);
        finish(master);

        // THEN the slave sets the error state
        TEST_ASSERT_TRUE(slave.has_error());
        TEST_ASSERT_EQUAL(I2CError::buffer_overflow, slave.error());
    }

    static void master_writes_too_many_bytes() {
        // GIVEN the slave is prepared to read some data
        slave.listen(ADDRESS);
        uint8_t rx_buffer = 0;
        slave.set_receive_buffer((uint8_t*)&rx_buffer, sizeof(rx_buffer));

        // WHEN a master attempts to write more bytes than the slave has available
        uint16_t tx_buffer = 0x1723;
        master.write_async(ADDRESS, (uint8_t*)&tx_buffer, sizeof(tx_buffer), true);
        finish(master);

        // THEN the slave receives as much data as fills the buffer
        TEST_ASSERT_EQUAL(0x23, rx_buffer);
        // AND the slave sets the error state
        TEST_ASSERT_TRUE(slave.has_error());
        TEST_ASSERT_EQUAL(I2CError::buffer_overflow, slave.error());
    }

    static void bit_error_during_write() {
        TEST_IGNORE_MESSAGE("Don't know how to trigger a bit error on demand");
        // GIVEN something pulls SDA LOW while the slave is transmitting

        // WHEN the slave tries to transmit some 1 bits
        slave.listen(ADDRESS);
        uint8_t tx_buffer = 0xFF;
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        read_1_byte();
        TEST_ASSERT_TRUE(master.has_error());

        // THEN the slave reports a bit error
        TEST_ASSERT_EQUAL(I2CError::bit_error, slave.error());
    }

    static void stop_listening_does_not_reset_error() {
        // GIVEN the slave is in an error state
        slave.listen(ADDRESS);
        read_2_bytes();
        TEST_ASSERT_EQUAL(I2CError::buffer_underflow, slave.error());

        // WHEN we stop listening
        slave.stop_listening();

        // THEN the error is not cleared
        TEST_ASSERT_EQUAL(I2CError::buffer_underflow, slave.error());
    }

    static void listen_resets_error() {
        // GIVEN the slave is in an error state
        slave.listen(ADDRESS);
        read_2_bytes();
        slave.stop_listening();
        TEST_ASSERT_TRUE(slave.has_error());

        // WHEN we call listen()
        slave.listen(ADDRESS);

        // THEN the error is cleared
        TEST_ASSERT_EQUAL(I2CError::ok, slave.error());
        TEST_ASSERT_FALSE(slave.has_error());
    }

    static void successful_transmit_resets_error() {
        // GIVEN the slave is in an error state
        slave.listen(ADDRESS);
        uint8_t tx_buffer = 0;
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        read_2_bytes();
        TEST_ASSERT_TRUE(slave.has_error());

        // WHEN the master reads some data successfully
        read_1_byte();

        // THEN the error is cleared
        TEST_ASSERT_EQUAL(I2CError::ok, slave.error());
        TEST_ASSERT_FALSE(slave.has_error());
    }

    static void successful_receive_resets_error() {
        // GIVEN the slave is in an error state
        slave.listen(ADDRESS);
        uint8_t rx_buffer = 0xFF;
        slave.set_receive_buffer(&rx_buffer, sizeof(rx_buffer));
        read_2_bytes();
        TEST_ASSERT_TRUE(slave.has_error());

        // WHEN the master writes some data successfully
        uint8_t tx_buffer = 0xBA;
        master.write_async(ADDRESS, (uint8_t*)&tx_buffer, sizeof(tx_buffer), true);
        finish(master);
        TEST_ASSERT_EQUAL(tx_buffer, rx_buffer);

        // THEN the error is cleared
        TEST_ASSERT_EQUAL(I2CError::ok, slave.error());
        TEST_ASSERT_FALSE(slave.has_error());
    }

    void test() final {
        RUN_TEST(master_reads_before_slave_sets_transmit_buffer);
        RUN_TEST(master_reads_too_many_bytes);
        RUN_TEST(master_writes_before_slave_sets_receive_buffer);
        RUN_TEST(master_writes_too_many_bytes);
        RUN_TEST(bit_error_during_write);
        RUN_TEST(stop_listening_does_not_reset_error);
        RUN_TEST(listen_resets_error);
        RUN_TEST(successful_transmit_resets_error);
        RUN_TEST(successful_receive_resets_error);
    }

    SlaveLogicTest() : LoopbackTestBase(__FILE__) {};
};

// Define statics
I2CMaster& e2e::loopback::logic::SlaveLogicTest::master = Master;
I2CSlave& e2e::loopback::logic::SlaveLogicTest::slave = Slave1;

} // signals
} // loopback
} // e2e

#endif //TEENSY4_I2C_TEST_E2E_SLAVE_LOGIC_H
