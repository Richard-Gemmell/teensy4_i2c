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

namespace e2e {

class E2ETestBase : public TestSuite {
public:
    // WARNING: Changing pins can affect the order of edges.
    // SDA on pin 23 and SCL on pin 22 works Ok.
    // SDA on pin 21 and SCL on pin 20 works Ok.
    const static uint32_t PIN_SNIFF_SDA = 21;
    const static uint32_t PIN_SNIFF_SCL = 20;

    static bus_trace::BusRecorder recorder;
    static common::hal::TeensyClock clock;
    static const size_t MAX_EVENTS = 1024;
    static bus_trace::BusEvent events[MAX_EVENTS];

    explicit E2ETestBase(const char* test_file_name)
            : TestSuite(test_file_name) {
    };

    void setUp() override {
        recorder.set_callbacks([](){recorder.add_event(false);}, [](){recorder.add_event(true);});
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
bus_trace::BusRecorder e2e::E2ETestBase::recorder(PIN_SNIFF_SDA, PIN_SNIFF_SCL); // NOLINT(cppcoreguidelines-interfaces-global-init)
common::hal::TeensyClock e2e::E2ETestBase::clock;
bus_trace::BusEvent e2e::E2ETestBase::events[MAX_EVENTS];
#endif //TEENSY4_I2C_E2E_TEST_BASE_H
