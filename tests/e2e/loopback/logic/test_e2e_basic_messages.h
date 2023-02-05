// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_BASIC_MESSAGES_H
#define TEENSY4_I2C_TEST_E2E_BASIC_MESSAGES_H

#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"
#include "e2e/loopback/loopback_test_base.h"
#include <imx_rt1060/imx_rt1060_i2c_driver.h>
#include <bus_trace/bus_trace_builder.h>

namespace e2e {
namespace loopback {
namespace logic {

// Tests the most common scenarios.
class BasicMessagesTest : public LoopbackTestBase {
public:
    // These 2 byte values contain all four bit transitions 0->0, 0->1, 1->1 and 1->0
    // The first and last bit of each byte affect the edges for ACKs
    // so one byte is the inverse of the other.
    const static uint8_t BYTE_A = 0x58; // 0101 1000
    const static uint8_t BYTE_B = 0xA7; // 1010 0111
    const static uint8_t ADDRESS = 0x53;// 0101 0011
    const static bool WRITE = bus_trace::BusTraceBuilder::WRITE;
    const static bool READ = bus_trace::BusTraceBuilder::READ;
    static I2CMaster* master;
    static I2CSlave* slave;
    static uint32_t frequency;

    void setUp() override {
        LoopbackTestBase::setUp();
        if (frequency == 400'000) {
            Loopback::enable_pullup(Loopback::PIN_SCL_120_ns);
            Loopback::enable_pullup(Loopback::PIN_SDA_120_ns);
        } else if (frequency == 1'000'000) {
            Loopback::enable_pullup(Loopback::PIN_SCL_FASTEST);
            Loopback::enable_pullup(Loopback::PIN_SDA_FASTEST);
        }
    }

    static void master_reads_multiple_bytes_successfully() {
        // Shows that the Master sends NACK instead of ACK to the last byte
        // of a message. It sends ACK for the others.
        // See I2C Specification section 3.1.6 item 5
        //
        // Shows that the Slave writes bytes correctly including all
        // four bit transitions. 0->0, 0->1, 1->1, 1->0
        bus_trace::BusTrace trace(MAX_EVENTS);
        const uint8_t tx_buffer[] = {0x00, BYTE_A, BYTE_B};
        slave->set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
        uint8_t rx_buffer[] = {0x00, 0x00, 0x00};

        // WHEN the master receives several bytes from the slave
        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [&rx_buffer](){
            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
        });

        // THEN the slave writes all bytes correctly
        // AND the master ACKS intermediate bytes but NACKs the final byte
        // AND the master received the data successfully
        bus_trace::BusTrace expected_trace(MAX_EVENTS);
        bus_trace::BusTraceBuilder builder(expected_trace, bus_trace::BusTraceBuilder::TimingStrategy::Min, common::i2c_specification::StandardMode);
        builder.bus_initially_idle().start_bit()
                // Slave replies to address with ACK
                .address_byte(ADDRESS, READ).ack()
                // Master ACKs each byte except the last.
                .data_byte(0x00).ack()
                .data_byte(BYTE_A).ack()
                // Master sends NACK after the last byte to signal end of message.
                // See I2C Specification section 3.1.6 item 5
                .data_byte(BYTE_B).nack()
                .stop_bit();
//        print_detailed_trace(trace);
        bus_trace::BusTrace normalised_trace = trace.to_message();
//        print_traces(normalised_trace, expected_trace);
        size_t compare = normalised_trace.compare_messages(expected_trace);
        TEST_ASSERT_EQUAL_UINT32(SIZE_MAX, compare);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));
    }

    static void master_writes_multiple_bytes_successfully() {
        // Shows that the Slave sends ACK after each byte.
        //
        // Shows that the Master writes bytes correctly including all
        // four bit transitions. 0->0, 0->1, 1->1, 1->0
        bus_trace::BusTrace trace(events, MAX_EVENTS);
        const uint8_t tx_buffer[] = {BYTE_A, BYTE_B};
        uint8_t rx_buffer[] = {0x00, 0x00};
        slave->set_receive_buffer(rx_buffer, sizeof(rx_buffer));

        // WHEN the master transmits several bytes to the slave
        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [&tx_buffer](){
            master->write_async(ADDRESS, tx_buffer, sizeof(tx_buffer), true);
        });

        // THEN the master writes all bytes correctly.
        // AND the slave ACKS every byte
        // AND the slave received the data successfully
        bus_trace::BusEvent expected_events[MAX_EVENTS];
        bus_trace::BusTrace expected_trace(expected_events, MAX_EVENTS);
        bus_trace::BusTraceBuilder builder(expected_trace, bus_trace::BusTraceBuilder::TimingStrategy::Min, common::i2c_specification::StandardMode);
        builder.bus_initially_idle().start_bit()
                .address_byte(ADDRESS, WRITE).ack()
                .data_byte(BYTE_A).ack()
                .data_byte(BYTE_B).ack()
                .stop_bit();
//        print_traces(trace, expected_trace);
        size_t compare = trace.compare_messages(expected_trace);
        TEST_ASSERT_EQUAL_UINT32(SIZE_MAX, compare);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));
    }

    static void test_suite(I2CMaster& master_, I2CSlave& slave_) {
        master = &master_;
        slave = &slave_;
        RUN_TEST(master_reads_multiple_bytes_successfully);
        RUN_TEST(master_writes_multiple_bytes_successfully);
    }

    void test() final {
        // Run all combinations of supported frequencies
        // and supported I2C ports
        Serial.println("Testing at 100 kHz");
        frequency = 100'000;
        Serial.println("Testing Master & Slave1");
        test_suite(Master, Slave1);
        Serial.println("Testing Master1 & Slave2");
        test_suite(Master1, Slave2);
        Serial.println("Testing Master2 & Slave");
        test_suite(Master2, Slave);
        Serial.println(".");

        Serial.println("Testing at 400 kHz");
        frequency = 400'000;
        Serial.println("Testing Master & Slave1");
        test_suite(Master, Slave1);
        Serial.println("Testing Master1 & Slave2");
        test_suite(Master1, Slave2);
        Serial.println("Testing Master2 & Slave");
        test_suite(Master2, Slave);
        Serial.println(".");

        Serial.println("Testing at 1 MHz");
        frequency = 1'000'000;
        Serial.println("Testing Master & Slave1");
        test_suite(Master, Slave1);
        Serial.println("Testing Master1 & Slave2");
        test_suite(Master1, Slave2);
        Serial.println("Testing Master2 & Slave");
        test_suite(Master2, Slave);
    }

    BasicMessagesTest() : LoopbackTestBase(__FILE__) {};
};

// Define statics
I2CMaster* e2e::loopback::logic::BasicMessagesTest::master;
I2CSlave* e2e::loopback::logic::BasicMessagesTest::slave;
uint32_t e2e::loopback::logic::BasicMessagesTest::frequency;

} // signals
} // loopback
} // e2e

#endif //TEENSY4_I2C_TEST_E2E_BASIC_MESSAGES_H
