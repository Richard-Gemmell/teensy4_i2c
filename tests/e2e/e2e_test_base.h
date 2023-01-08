// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_E2E_TEST_BASE_H
#define TEENSY4_I2C_E2E_TEST_BASE_H

#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"
#include <i2c_driver.h>
#include <common/hal/arduino/arduino_pin.h>
#include <common/hal/teensy/teensy_clock.h>
#include <bus_trace/bus_event.h>
#include <bus_trace/bus_trace.h>
#include <bus_trace/bus_recorder.h>
#include <analysis/i2c_timing_analyser.h>
#include <analysis/i2c_timing_analysis.h>

namespace e2e {

class E2ETestBase : public TestSuite {
public:
    // WARNING: Changing pins can affect the order of edges.
    // SDA on pin 23 and SCL on pin 22 works Ok.
    // SDA on pin 21 and SCL on pin 20 works Ok.
    constexpr static uint32_t PIN_SNIFF_SDA = 21;   // GPIO1 and GPIO6 bit 27
    constexpr static uint32_t PIN_SNIFF_SCL = 20;   // GPIO1 and GPIO6 bit 26

    // These 2 byte values contain all four bit transitions 0->0, 0->1, 1->1 and 1->0
    // The first and last bit of each byte affect the edges for ACKs
    // so one byte is the inverse of the other.
    const static uint8_t BYTE_A = 0x58; // 0101 1000
    const static uint8_t BYTE_B = 0xA7; // 1010 0111
    const static uint8_t ADDRESS = 0x53;

    static bus_trace::BusRecorder recorder;
    static common::hal::TeensyClock clock;
    constexpr static size_t MAX_EVENTS = 1024;
    static bus_trace::BusEvent events[MAX_EVENTS];

    explicit E2ETestBase(const char* test_file_name)
            : TestSuite(test_file_name) {
    };

    void setUp() override {
        recorder.set_callback([](){recorder.add_event();});
        pinMode(PIN_SNIFF_SDA, INPUT);
        pinMode(PIN_SNIFF_SCL, INPUT);
    }

    void tearDown() override {
        pinMode(PIN_SNIFF_SCL, INPUT_DISABLE);
        pinMode(PIN_SNIFF_SDA, INPUT_DISABLE);
    }

    static void trace_i2c_transaction(
            I2CMaster* master,
            uint32_t frequency,
            I2CSlave* slave,
            uint8_t slave_address,
            bus_trace::BusTrace& trace,
            const std::function<void()>& transaction) {
        trace_i2c_transaction(*master, frequency, *slave, slave_address, trace, [transaction](I2CMaster& masterRef, I2CSlave& slaveRef){
            transaction();
        });
    }

    static void trace_i2c_transaction(
            I2CMaster& master,
            uint32_t frequency,
            I2CSlave& slave,
            uint8_t slave_address,
            bus_trace::BusTrace& trace,
            const std::function<void(I2CMaster& master, I2CSlave& slave)>& transaction) {
        // Connect the Master and Slave
        master.begin(frequency);
        slave.listen(slave_address);
        // Start a recording
        recorder.start(trace);

        // Trigger the I2C transaction
        transaction(master, slave);
        finish(master);         // Wait for the I2C transaction to end.
        delayNanoseconds(1000); // Wait for the recorder to catch the final edge.

        // Clean up
        slave.stop_listening();
        master.end();
        recorder.stop();
    }

    static analysis::I2CTimingAnalysis analyse_read_transaction(
            I2CMaster* master, I2CSlave* slave,
            uint32_t frequency,
            uint32_t sda_rise_time, uint32_t scl_rise_time) {
        delayMicroseconds(100); // Necessary to avoid intermittent failures
        bus_trace::BusTrace trace(&clock, MAX_EVENTS);

        const uint8_t tx_buffer[] = {BYTE_A, BYTE_B};
        slave->set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
        uint8_t rx_buffer[] = {0x00, 0x00};

        // Master reads from slave
        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [master, &rx_buffer](){
            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
        });
//        print_detailed_trace(trace);
        // Ensure the read succeeded
        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));

        bus_trace::BusTrace normalised = trace.to_message(false, true);
        return analysis::I2CTimingAnalyser::analyse(normalised, sda_rise_time, scl_rise_time);
    }

    static analysis::I2CTimingAnalysis analyse_write_transaction(
        I2CMaster* master, I2CSlave* slave,
        uint32_t frequency,
        uint32_t sda_rise_time, uint32_t scl_rise_time) {
        bus_trace::BusTrace trace(&clock, MAX_EVENTS);

        const uint8_t tx_buffer[] = {BYTE_A, BYTE_B};
        uint8_t rx_buffer[] = {0x00, 0x00};
        slave->set_receive_buffer(rx_buffer, sizeof(rx_buffer));

        // Master writes to the slave
        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [master, &tx_buffer](){
            master->write_async(ADDRESS, tx_buffer, sizeof(tx_buffer), true);
        });
//        print_detailed_trace(trace);
        // Ensure the write succeeded
        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));

        return analysis::I2CTimingAnalyser::analyse(trace, sda_rise_time, scl_rise_time);
    }

    static analysis::I2CTimingAnalysis analyse_repeated_read_transaction(
        I2CMaster* master, I2CSlave* slave,
        uint32_t frequency,
        uint32_t sda_rise_time, uint32_t scl_rise_time,
        bool stop_after_first_read = false) {
        bus_trace::BusTrace trace(&clock, MAX_EVENTS);

        const uint8_t tx_buffer[] = {BYTE_A, BYTE_B};
        slave->set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
        uint8_t rx_buffer[] = {0x00, 0x00};

        // Master reads from slave
        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [master, &rx_buffer, &stop_after_first_read](){
            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), stop_after_first_read);
            finish(*master);
            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
        });
//        print_detailed_trace(trace);
        // Ensure the read succeeded
        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));

        return analysis::I2CTimingAnalyser::analyse(trace, sda_rise_time, scl_rise_time);
    }

    static void print_detailed_trace(const bus_trace::BusTrace& trace) {
        Serial.println(trace);
        for (size_t i = 0; i < trace.event_count(); ++i) {
            Serial.printf("  Index %d: delta %d ns\n", i, common::hal::TeensyTimestamp::ticks_to_nanos(trace.event(i)->delta_t_in_ticks));
        }
    }

    static void print_traces(bus_trace::BusTrace& actual, bus_trace::BusTrace& expected) {
        Serial.println("Actual:");
        Serial.print(actual.to_message());
        Serial.println("Expected:");
        Serial.print(expected.to_message());
    }

    static void finish(I2CMaster& master) {
        elapsedMillis timeout;
        while (timeout < 200) {
            if (master.finished()) {
                return;
            }
        }
        Serial.println("Master: ERROR timed out waiting for transfer to finish.");
    }
};
}
// Define statics
bus_trace::BusRecorder e2e::E2ETestBase::recorder(PIN_SNIFF_SDA, PIN_SNIFF_SCL);
common::hal::TeensyClock e2e::E2ETestBase::clock;
bus_trace::BusEvent e2e::E2ETestBase::events[MAX_EVENTS];
#endif //TEENSY4_I2C_E2E_TEST_BASE_H
