// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_BUS_RECORDER_H
#define TEENSY4_I2C_TEST_BUS_RECORDER_H

#include <unity.h>
#include <Arduino.h>
#include <bus_trace/bus_trace_builder.h>
#include "utils/test_suite.h"
#include "e2e/loopback/loopback_test_base.h"

namespace e2e {
namespace loopback {
namespace bus_trace_tests {

class BusRecorderTest : public LoopbackTestBase {
    const static uint8_t ADDRESS = 0x53;
    const static uint8_t BYTE_A = 0x58; // 0101 1000
public:
    void tearDown() override {
        pinMode(PIN_WIRE_SCL, INPUT_DISABLE);
        pinMode(PIN_WIRE_SDA, INPUT_DISABLE);
        LoopbackTestBase::tearDown();
    }

    // Records an I2C trace and compares it to values measured with
    // an oscilloscope
    static void test_i2c_timings() {
        Loopback::enable_pullup(Loopback::PIN_SCL_FASTEST);
        Loopback::enable_pullup(Loopback::PIN_SDA_FASTEST);
        bus_trace::BusTrace trace(&clock, MAX_EVENTS);
        uint8_t tx = BYTE_A;
        Slave1.set_transmit_buffer(&tx, sizeof(tx));
        uint8_t rx = 0x00;

        // WHEN the master receives several bytes from the slave
        trace_i2c_transaction(&Master, 1'000'000, &Slave1, ADDRESS, trace, [&rx](){
            Master.read_async(ADDRESS, &rx, sizeof(rx), true);
        });

        // THEN the slave writes all bytes correctly
        // AND the master ACKS intermediate bytes but NACKs the final byte
        bus_trace::BusEvent expected_events[MAX_EVENTS];
        bus_trace::BusTrace expected_trace(expected_events, MAX_EVENTS);
        bus_trace::BusTraceBuilder builder(expected_trace, bus_trace::BusTraceBuilder::TimingStrategy::Min, common::i2c_specification::StandardMode);
        builder.bus_initially_idle().start_bit()
                // Slave replies to address with ACK
                .address_byte(ADDRESS, bus_trace::BusTraceBuilder::READ).ack()
                // Master sends NACK after the last byte to signal end of message.
                .data_byte(BYTE_A).nack()
                .stop_bit();
//        print_traces(trace, expected_trace);
//        for (size_t i = 0; i < trace.event_count(); ++i) {
//            Serial.printf("Index %d: delta %d ns\n", i, common::hal::TeensyTimestamp::ticks_to_nanos(trace.event(i)->delta_t_in_ticks));
//        }
        size_t compare = trace.compare_messages(expected_trace);    // Make sure we got the edges in the right order.
        TEST_ASSERT_EQUAL_UINT32(SIZE_MAX, compare);
        TEST_ASSERT_EQUAL_UINT8(BYTE_A, rx);

        uint32_t measurements[54] = {
                0, 0, 456, 214, 248,
                535, 210, 255, 536, 216, // 9
                248, 536, 210, 256, 534,
                456, 536, 214, 248, 536, // 19
                466, 536, 462, 536, 10,
                458, 536, 464, 534, 20, // 29
                442, 538, 10, 456, 535,
                22, 444, 536, 462, 538, // 39
                10, 458, 536, 462, 536,
                464, 538, 20, 440, 536, // 49
                208, 258, 584, 416
        };
        // Anything gap between 2 edges gets recorded as 64 nanos however
        // short it was. We have to allow errors this big when both edges
        // fall in quick succession
        uint32_t allowed_error = 65;
        bool failed = false;
        // Ignore the first 2 events. The second is the first edge of the message.
        // The third is the first one where we can predict the delay
        for (size_t i = 2; i < expected_trace.event_count(); ++i) {
            uint32_t actual = common::hal::TeensyTimestamp::ticks_to_nanos(trace.event(i)->delta_t_in_ticks);
            auto error = (uint32_t)fabs(measurements[i]*1.0 - actual*1.0);
            if(error > allowed_error) {
                Serial.printf("Entry %d expected %d ns +/- %d but got %d. Error %d.\n", i, measurements[i], allowed_error, actual, error);
                failed = true;
            }
        }
        if(failed) {
            TEST_FAIL_MESSAGE("BusRecorder times don't match oscilloscope.");
        }
    }

    // If both lines are changed in quick succession
    // then the BusRecorder gets the order of SDA and SCL wrong.
    // Note: I wasn't able to make edges disappear entirely with this trick.
    static void test_reorder_events() {
        pinMode(PIN_WIRE_SCL, OUTPUT_OPENDRAIN);
        pinMode(PIN_WIRE_SDA, OUTPUT_OPENDRAIN);
        Loopback::enable_pullup(Loopback::PIN_SCL_FASTEST);
        Loopback::enable_pullup(Loopback::PIN_SDA_FASTEST);

        bus_trace::BusTrace trace(MAX_EVENTS);
        // It takes about 30 ns for the line to rise to the point where the edge is detectable
        int interval = 14;  // Edges are 35 ns apart. BusRecorder flips the order of the pins.
//        int interval = 16;  // Edges are 40 ns apart. BusRecorder gets pin order correct.

        // WHEN we record 2 edges in quick succession
        recorder.start(trace);
        digitalWriteFast(PIN_WIRE_SDA, true);
        delayNanoseconds(interval);
        digitalWriteFast(PIN_WIRE_SCL, true);
        delayNanoseconds(300);
        recorder.stop();

        // THEN the BusRecorder gets the order of SDA and SCL wrong.
//        trace.printTo(Serial);
        TEST_ASSERT_EQUAL_UINT32(3, trace.event_count());
        TEST_ASSERT_NOT_EQUAL(bus_trace::BusEventFlags::SDA_LINE_CHANGED | bus_trace::BusEventFlags::SDA_LINE_STATE, trace.event(1)->flags);
    }

    // Include all the tests here
    void test() final {
        RUN_TEST(test_i2c_timings);
        RUN_TEST(test_reorder_events);
    }

    BusRecorderTest() : LoopbackTestBase(__FILE__) {};
};

}
}
}

#endif //TEENSY4_I2C_TEST_BUS_RECORDER_H
