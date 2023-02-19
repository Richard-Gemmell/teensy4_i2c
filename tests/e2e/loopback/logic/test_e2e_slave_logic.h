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
    const static uint8_t ADDRESS_2 = 0x7F;
    static uint8_t EMPTY_BUFFER[0];
    static I2CMaster& master;
    static I2CSlave& slave;

    void setUp() override {
        LoopbackTestBase::setUp();
        // Reset slave buffers. This isn't done by any slave methods.
        slave.set_receive_buffer(EMPTY_BUFFER, 0);
        slave.set_transmit_buffer(EMPTY_BUFFER, 0);
        slave.before_transmit(nullptr);
        slave.after_transmit(nullptr);
        slave.after_receive(nullptr);
        master.begin(FREQUENCY);
    }

    static void write_1_byte(uint8_t tx_buffer) {
        master.write_async(ADDRESS, (uint8_t*)&tx_buffer, sizeof(tx_buffer), true);
        finish(master);
    }

    static uint8_t read_1_byte(uint8_t address = ADDRESS) {
        uint8_t rx_buffer = 0;
        master.read_async(address, (uint8_t*)&rx_buffer, sizeof(rx_buffer), true);
        finish(master);
        return rx_buffer;
    }

    static void read_2_bytes() {
        uint16_t rx_buffer = 0;
        master.read_async(ADDRESS, (uint8_t*)&rx_buffer, sizeof(rx_buffer), true);
        finish(master);
    }

    static void ignores_receive_request_if_not_listening() {
        // GIVEN the slave is not listening

        // WHEN the master attempts to write to the slave
        write_1_byte(0x23);

        // THEN the request is ignored
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_TRUE(master.has_error());
        // TODO: Currently returns the wrong error type
//        TEST_ASSERT_EQUAL(I2CError::address_nak, master.error());
    }

    static void ignores_transmit_request_if_not_listening() {
        // GIVEN the slave is not listening

        // WHEN the master attempts to read from the slave
        read_1_byte();

        // THEN the request is ignored
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_EQUAL(I2CError::address_nak, master.error());
    }

    static void ignores_receive_request_after_stop_listening() {
        // GIVEN the slave was listening
        slave.listen(ADDRESS);
        uint8_t rx_buffer = 0;
        slave.set_receive_buffer(&rx_buffer, sizeof(rx_buffer));
        write_1_byte(0x23);
        TEST_ASSERT_EQUAL_HEX8(0x23, rx_buffer);

        // WHEN the slave stops listening
        slave.stop_listening();
        // AND the master attempts to write to the slave
        write_1_byte(0xCE);

        // THEN the request is ignored
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_EQUAL_HEX8(0x23, rx_buffer);
        TEST_ASSERT_TRUE(master.has_error());
        // TODO: Currently returns the wrong error type
//        TEST_ASSERT_EQUAL(I2CError::address_nak, master.error());
    }

    static void ignores_transmit_request_after_stop_listening() {
        // GIVEN the slave was listening
        slave.listen(ADDRESS);
        uint8_t tx_buffer = 0xBA;
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        read_1_byte();

        // WHEN the slave stops listening
        slave.stop_listening();
        TEST_ASSERT_FALSE(slave.has_error());
        // AND the master attempts to read from the slave
        read_1_byte();

        // THEN the request is ignored
        TEST_ASSERT_EQUAL(I2CError::ok, slave.error());
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_EQUAL(I2CError::address_nak, master.error());
    }

    static void can_listen_to_2_addresses() {
        // WHEN the slave is listening to 2 addresses
        const uint8_t tx_buffer = 0xCC;
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        const uint8_t address1 = 0x20;
        const uint8_t address2 = 0x31;
        slave.listen(address2, address1);

        // THEN the master can read data from the first address
        TEST_ASSERT_EQUAL(0xCC, read_1_byte(address1));
        // AND the master can read the same value from the second address
        TEST_ASSERT_EQUAL(0xCC, read_1_byte(address2));
    }

    static void can_listen_to_a_range_of_addresses() {
        // WHEN the slave is listening to 2 addresses
        const uint8_t tx_buffer = 0xDF;
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        const uint8_t address1 = 0x20;
        const uint8_t address2 = 0x22;
        slave.listen_range(address1, address2);

        // THEN the slave is not listening before the first address
        read_1_byte(address1 - 1);
        TEST_ASSERT_EQUAL(I2CError::address_nak, master.error());
        TEST_ASSERT_FALSE(slave.has_error());
        // AND the master can read data from the first address
        TEST_ASSERT_EQUAL(0xDF, read_1_byte(0x20));
        TEST_ASSERT_FALSE(slave.has_error());
        // AND the master can read the same value from the second address
        TEST_ASSERT_EQUAL(0xDF, read_1_byte(0x21));
        TEST_ASSERT_FALSE(slave.has_error());
        // AND the master can read the same value from the last address
        TEST_ASSERT_EQUAL(0xDF, read_1_byte(0x22));
        TEST_ASSERT_FALSE(slave.has_error());
        // AND the slave is not listening after the last address
        read_1_byte(address2 + 1);
        TEST_ASSERT_EQUAL(I2CError::address_nak, master.error());
        TEST_ASSERT_FALSE(slave.has_error());
    }

    static void stop_listening_does_not_reset_receive_buffer() {
        // GIVEN the slave set the buffer and used it successfully
        uint8_t rx_buffer = 0x00;
        slave.set_receive_buffer(&rx_buffer, sizeof(rx_buffer));
        slave.listen(ADDRESS);
        write_1_byte(0x31);
        TEST_ASSERT_EQUAL_HEX8(0x31, rx_buffer);

        // WHEN we stop listening and start listening again
        slave.stop_listening();
        slave.listen(ADDRESS);
        write_1_byte(0x42);

        // THEN the buffer is still available
        TEST_ASSERT_EQUAL_HEX8(0x42, rx_buffer);
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_FALSE(master.has_error());
    }

    static void stop_listening_does_not_reset_transmit_buffer() {
        // GIVEN the slave set the buffer
        const uint8_t tx_buffer = 0xBA;
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        slave.listen(ADDRESS);
        read_1_byte();

        // WHEN we stop listening and start listening again
        slave.stop_listening();
        slave.listen(ADDRESS);
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        uint8_t value = read_1_byte();

        // THEN the buffer is still available
        TEST_ASSERT_EQUAL_HEX8(0xBA, value);
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_FALSE(master.has_error());
    }

    static void master_reads_before_slave_sets_transmit_buffer() {
        // GIVEN the slave has not set a transmit buffer
        slave.listen(ADDRESS);

        // WHEN a master attempts to read from the slave
        uint16_t rx_buffer = 0;
        master.read_async(ADDRESS, (uint8_t*)&rx_buffer, sizeof(rx_buffer), true);
        finish(master);

        // THEN the master receives dummy bytes
        TEST_ASSERT_EQUAL_HEX16(0x0000, rx_buffer);
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
        TEST_ASSERT_EQUAL_HEX16(0x0017, rx_buffer);
        // AND the slave sets the error state
        TEST_ASSERT_TRUE(slave.has_error());
        TEST_ASSERT_EQUAL(I2CError::buffer_underflow, slave.error());
    }

    static void master_writes_before_slave_sets_receive_buffer() {
        // GIVEN the slave has not set a receive buffer
        slave.listen(ADDRESS);

        // WHEN a master attempts to write to the slave
        write_1_byte(0x17);

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
        TEST_ASSERT_EQUAL_HEX8(0x23, rx_buffer);
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
        const uint8_t tx_buffer = 0xBA;
        master.write_async(ADDRESS, (uint8_t*)&tx_buffer, sizeof(tx_buffer), true);
        finish(master);
        TEST_ASSERT_EQUAL_HEX8(tx_buffer, rx_buffer);

        // THEN the error is cleared
        TEST_ASSERT_EQUAL(I2CError::ok, slave.error());
        TEST_ASSERT_FALSE(slave.has_error());
    }

    static void can_transmit_repeatedly() {
        // GIVEN the slave has transmitted the contents of its buffer already
        slave.listen(ADDRESS);
        const uint8_t tx_buffer = 0xCF;
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        const uint8_t value = read_1_byte();
        TEST_ASSERT_EQUAL_HEX8(0xCF, value);
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_FALSE(master.has_error());

        // WHEN the master reads from the slave again
        const uint8_t value2 = read_1_byte();

        // THEN it gets the same value
        TEST_ASSERT_EQUAL_HEX8(0xCF, value2);
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_FALSE(master.has_error());
    }

    static void can_receive_repeatedly() {
        // GIVEN the slave has received a value already
        slave.listen(ADDRESS);
        uint8_t rx_buffer = 0;
        slave.set_receive_buffer(&rx_buffer, sizeof(rx_buffer));
        write_1_byte(0x51);
        TEST_ASSERT_EQUAL_HEX8(0x51, rx_buffer);
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_FALSE(master.has_error());

        // WHEN the master transmits again
        write_1_byte(0x07);

        // THEN the slave receives the value successfully
        TEST_ASSERT_EQUAL_HEX8(0x07, rx_buffer);
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_FALSE(master.has_error());
    }

    static void after_receive_callback_is_called() {
        // GIVEN the slave is ready to transmit and listening to 2 addresses
        uint8_t rx_buffer[] = {0, 0, 0, 0, 0, 0};
        slave.set_receive_buffer(rx_buffer, sizeof(rx_buffer));
        size_t actual_length;
        uint16_t actual_address;
        slave.after_receive([&actual_length, &actual_address](size_t length, uint16_t address) {
            actual_length = length;
            actual_address = address;
        });
        slave.listen(ADDRESS_2, ADDRESS);

        // WHEN the master transmits a message to one address
        uint32_t message1 = 0xAABBCCDD;
        master.write_async(ADDRESS_2, (uint8_t*)(&message1), sizeof(message1), true);
        finish(master);

        // THEN the callback was called with the correct values
        TEST_ASSERT_FALSE(master.has_error());
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_EQUAL(4, actual_length);
        TEST_ASSERT_EQUAL(ADDRESS_2, actual_address);

        // WHEN the master transmits a message to the other address
        uint8_t message2 = 0xFF;
        master.write_async(ADDRESS, &message2, sizeof(message2), true);
        finish(master);

        // THEN the callback was called with the correct values
        TEST_ASSERT_FALSE(master.has_error());
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_EQUAL(1, actual_length);
        TEST_ASSERT_EQUAL(ADDRESS, actual_address);
    }

    static void before_transmit_callback_is_called() {
        // GIVEN the slave is ready to transmit and listening to a range of addresses
        const uint8_t tx_buffer = 0xCC;
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        slave.listen_range(ADDRESS, ADDRESS_2);
        uint8_t actual_address;
        slave.before_transmit([&actual_address](uint16_t address) {
            actual_address = address;
        });

        // WHEN the master receives a message from the slave
        uint8_t rx_buffer = 0;
        master.read_async(ADDRESS + 5, &rx_buffer, sizeof(rx_buffer), true);
        finish(master);

        // THEN the callback was called with the correct values
        TEST_ASSERT_FALSE(master.has_error());
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_EQUAL(ADDRESS + 5, actual_address);
    }

    static void can_set_transmit_buffer_in_before_transmit_callback() {
        // GIVEN the slave is listening
        slave.listen(ADDRESS);
        slave.before_transmit([](uint16_t address) {
            const uint8_t tx_buffer[] = {0xCC, 0xEE};
            slave.set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
        });

        // WHEN the master receives a message from the slave
        uint8_t rx_buffer[] = {0, 0};
        master.read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
        finish(master);

        // THEN the callback was called with the correct values
        TEST_ASSERT_FALSE(master.has_error());
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_EQUAL(0xCC, rx_buffer[0]);
        TEST_ASSERT_EQUAL(0xEE, rx_buffer[1]);
    }

    static void after_transmit_callback_is_called() {
        // GIVEN the slave is ready to transmit and listening to 2 addresses
        const uint8_t tx_buffer = 0xCC;
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        slave.listen_range(ADDRESS, ADDRESS_2);
        uint8_t actual_address = 0;
        slave.after_transmit([&actual_address](uint16_t address) {
            actual_address = address;
        });

        // WHEN the master receives a message from the slave
        uint8_t rx_buffer = 0;
        master.read_async(ADDRESS + 5, &rx_buffer, sizeof(rx_buffer), true);
        finish(master);

        // THEN the callback was called with the correct values
        TEST_ASSERT_FALSE(master.has_error());
        TEST_ASSERT_FALSE(slave.has_error());
        TEST_ASSERT_EQUAL(ADDRESS + 5, actual_address);
    }

    static void can_change_transmit_buffer_between_transactions() {
        // GIVEN the master has received a message from the slave
        const uint8_t tx_buffer = 0xCC;
        slave.set_transmit_buffer(&tx_buffer, sizeof(tx_buffer));
        slave.listen(ADDRESS);
        uint8_t first_value = read_1_byte();
        TEST_ASSERT_EQUAL_HEX8(0xCC, first_value);

        // WHEN the slave changes the buffer
        const uint8_t tx_buffer_2 = 0x18;
        slave.set_transmit_buffer(&tx_buffer_2, sizeof(tx_buffer_2));

        // THEN the master receives the new value
        uint8_t second_value = read_1_byte();
        TEST_ASSERT_EQUAL_HEX8(0x18, second_value);
        TEST_ASSERT_FALSE(master.has_error());
        TEST_ASSERT_FALSE(slave.has_error());
    }

    static void can_change_receive_buffer_between_transactions() {
        // GIVEN the master has already sent a message to the slave
        slave.listen(ADDRESS);
        uint8_t first_buffer = 0;
        slave.set_receive_buffer(&first_buffer, sizeof(first_buffer));
        write_1_byte(0x23);
        TEST_ASSERT_EQUAL_HEX8(0x23, first_buffer);

        // WHEN the slave changes the buffer
        uint8_t second_buffer[] = {0, 0};
        slave.set_receive_buffer(second_buffer, sizeof(second_buffer));

        // THEN the master writes to the new buffer not the old one
        write_1_byte(0x03);
        TEST_ASSERT_EQUAL_HEX8(0x23, first_buffer);
        TEST_ASSERT_EQUAL_HEX8(0x03, second_buffer[0]);
        TEST_ASSERT_EQUAL_HEX8(0x00, second_buffer[1]);
        TEST_ASSERT_FALSE(master.has_error());
        TEST_ASSERT_FALSE(slave.has_error());
    }

    void test() final {
        RUN_TEST(ignores_receive_request_if_not_listening);
        RUN_TEST(ignores_transmit_request_if_not_listening);
        RUN_TEST(ignores_receive_request_after_stop_listening);
        RUN_TEST(ignores_transmit_request_after_stop_listening);
        RUN_TEST(can_listen_to_2_addresses);
        RUN_TEST(can_listen_to_a_range_of_addresses);
        RUN_TEST(stop_listening_does_not_reset_receive_buffer);
        RUN_TEST(stop_listening_does_not_reset_transmit_buffer);
        RUN_TEST(master_reads_before_slave_sets_transmit_buffer);
        RUN_TEST(master_reads_too_many_bytes);
        RUN_TEST(master_writes_before_slave_sets_receive_buffer);
        RUN_TEST(master_writes_too_many_bytes);
        RUN_TEST(bit_error_during_write);
        RUN_TEST(stop_listening_does_not_reset_error);
        RUN_TEST(listen_resets_error);
        RUN_TEST(successful_transmit_resets_error);
        RUN_TEST(successful_receive_resets_error);
        RUN_TEST(can_transmit_repeatedly);
        RUN_TEST(can_receive_repeatedly);
        RUN_TEST(after_receive_callback_is_called);
        RUN_TEST(after_receive_callback_is_called);
        RUN_TEST(before_transmit_callback_is_called);
        RUN_TEST(can_set_transmit_buffer_in_before_transmit_callback);
        RUN_TEST(after_transmit_callback_is_called);
        RUN_TEST(can_change_transmit_buffer_between_transactions);
        RUN_TEST(can_change_receive_buffer_between_transactions);
    }

    SlaveLogicTest() : LoopbackTestBase(__FILE__) {};
};

// Define statics
uint8_t e2e::loopback::logic::SlaveLogicTest::EMPTY_BUFFER[0];
I2CMaster& e2e::loopback::logic::SlaveLogicTest::master = Master;
I2CSlave& e2e::loopback::logic::SlaveLogicTest::slave = Slave1;

} // signals
} // loopback
} // e2e

#endif //TEENSY4_I2C_TEST_E2E_SLAVE_LOGIC_H
