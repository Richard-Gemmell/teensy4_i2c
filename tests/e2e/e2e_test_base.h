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
    const static uint32_t PIN_SNIFF_SDA = 23;
    const static uint32_t PIN_SNIFF_SCL = 22;

    static common::hal::ArduinoPin sda;
    static common::hal::ArduinoPin scl;
    static common::hal::TeensyClock clock;
    static const size_t MAX_EVENTS = 1024;
    static bus_trace::BusEvent events[MAX_EVENTS];

    explicit E2ETestBase(const char* test_file_name)
            : TestSuite(test_file_name) {
    };

    void setUp() override {
        pinMode(PIN_SNIFF_SDA, INPUT);
        pinMode(PIN_SNIFF_SCL, INPUT);
        sda.set_on_edge_isr([]() {
            sda.raise_on_edge();
        });
        scl.set_on_edge_isr([]() {
            scl.raise_on_edge();
        });
    }

    void tearDown() override {
        sda.set_on_edge_isr(nullptr);
        scl.set_on_edge_isr(nullptr);
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
        bus_trace::BusRecorder recorder(sda, scl, clock);
        recorder.start(trace);

        // Trigger the I2C transaction
        transaction(master, slave);

        // Clean up
        finish(master);
        slave.stop_listening();
        master.end();
        recorder.stop();
    }

    static void print_traces(bus_trace::BusTrace& actual, bus_trace::BusTrace& expected) {
        Serial.println("Actual:");
        Serial.print(actual);
        Serial.println("Expected:");
        Serial.print(expected);
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
common::hal::ArduinoPin e2e::E2ETestBase::sda(PIN_SNIFF_SDA); // NOLINT(cppcoreguidelines-interfaces-global-init)
common::hal::ArduinoPin e2e::E2ETestBase::scl(PIN_SNIFF_SCL); // NOLINT(cppcoreguidelines-interfaces-global-init)
common::hal::TeensyClock e2e::E2ETestBase::clock;
bus_trace::BusEvent e2e::E2ETestBase::events[MAX_EVENTS];
#endif //TEENSY4_I2C_E2E_TEST_BASE_H
