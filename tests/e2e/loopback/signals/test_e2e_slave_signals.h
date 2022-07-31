// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_SLAVE_SIGNALS_H
#define TEENSY4_I2C_TEST_E2E_SLAVE_SIGNALS_H

#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"
#include "e2e/e2e_test_base.h"

namespace e2e {
namespace loopback {
namespace signals {

// Checks that the driver gets the timings right when a slave writes to the bus
class SlaveSignalTest : public E2ETestBase {

public:
//    void setUp() override {
//        E2ETestBase::setUp();
//    }
//
//    void tearDown() override {
//        E2ETestBase::tearDown();
//    }

    const static uint8_t ADDRESS = 0x53;

    static void test_example_one() {
//        // GIVEN a master and slave
//        I2CMaster& master = Master;
//        I2CSlave& slave = Slave1;
//        bus_trace::BusTrace trace(events, MAX_EVENTS);
//
//        // WHEN the master reads a single byte from the slave
//        uint8_t tx_buffer[1] = {0x58};
//        slave.set_transmit_buffer(tx_buffer, 1);
//        uint8_t rx_buffer[1] = {0x00};
//        trace_i2c_transaction(master, 100'000U, slave, ADDRESS, trace, [&rx_buffer](I2CMaster& master, I2CSlave& slave){
//            master.read_async(ADDRESS, rx_buffer, 1, sizeof(rx_buffer));
//        });
//
//        bus_trace::BusEvent expected_events[MAX_EVENTS];
//        bus_trace::BusTrace expected_trace(expected_events, MAX_EVENTS);
//        bus_trace::BusTraceBuilder builder(expected_trace, bus_trace::BusTraceBuilder::TimingStrategy::Min, common::i2c_specification::StandardMode);
//        builder.bus_initially_idle();
//        builder.start_bit();
//        builder.address_byte(ADDRESS, true);
//        builder.ack();
//        builder.data_byte(0x58);
//        builder.nack();
//        builder.stop_bit();
//
//        print_traces(trace, expected_trace);
//
//        // THEN the data is received correctly
//        size_t compare = trace.compare_edges(expected_trace);
//        TEST_ASSERT_EQUAL_UINT32(SIZE_MAX, compare);
//        TEST_ASSERT_EQUAL(0x58, rx_buffer[0]);
        TEST_ASSERT_FALSE_MESSAGE(1, "Doesn't actually check any signals yet.");
    }

//    static void slave_releases_SDA_so_master_can_ACK() {
//        bus_trace::BusTrace trace(events, MAX_EVENTS);
//        uint8_t tx_buffer[] = {0x00, 0xFF};
//        slave->set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
//        uint8_t rx_buffer[] = {0x00, 0x00};
//
//        // WHEN the master reads a byte ending in a zero bit
//        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [&rx_buffer](){
//            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
//        });
//
//        bus_trace::BusEvent expected_events[MAX_EVENTS];
//        bus_trace::BusTrace expected_trace(expected_events, MAX_EVENTS);
//        bus_trace::BusTraceBuilder builder(expected_trace, bus_trace::BusTraceBuilder::TimingStrategy::Min, common::i2c_specification::StandardMode);
//        builder.bus_initially_idle().start_bit()
//                .address_byte(ADDRESS, true).ack()
//                .data_byte(0x00);
//        // THEN the slave must release SDA so the master can pull it down
////        expected_trace.add_event(200, bus_trace::BusEventFlags::SDA_LINE_CHANGED | bus_trace::BusEventFlags::SDA_LINE_STATE);
////        // AND the master pulls it low to send an ACK
////        expected_trace.add_event(1600, bus_trace::BusEventFlags::SDA_LINE_CHANGED);
////        expected_trace.add_event(3200, bus_trace::BusEventFlags::SCL_LINE_CHANGED | bus_trace::BusEventFlags::SCL_LINE_STATE);
////        expected_trace.add_event(4800, bus_trace::BusEventFlags::SCL_LINE_CHANGED);
//        builder.ack();
//        // AND we send the rest of the message
//        builder.data_byte(0xFF).nack()
//                .stop_bit();
//
//        // THEN the slave must release SDA so the master can pull it down
//        print_traces(trace, expected_trace);
//        size_t compare = trace.compare_edges(expected_trace);
//        TEST_ASSERT_EQUAL_UINT32(SIZE_MAX, compare);
//        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));
//    }
//
    static void test_example_two() {
        test_example_one();
    }

    void test() final {
        RUN_TEST(test_example_one);
//        RUN_TEST(test_example_two);
        // TODO: Slave ACKs master after a zero bit. Slave releases SDA before master pulls it low.
    }

    SlaveSignalTest() : E2ETestBase(__FILE__) {};
};

// Define statics
//int SlaveSignalTest::value;

} // signals
} // loopback
} // e2e
#endif //TEENSY4_I2C_TEST_E2E_SLAVE_SIGNALS_H
