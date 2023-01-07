// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_RISE_TIMES_H
#define TEENSY4_I2C_TEST_E2E_RISE_TIMES_H

#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"
#include "e2e/loopback/loopback.h"
#include <imx_rt1060/imx_rt1060_i2c_driver.h>
#include <line_test/line_tester.h>

namespace e2e {
namespace loopback {
namespace signals {

class RiseTimeTest : public TestSuite {
public:
    void tearDown() override {
        Master.end();
        Master.set_internal_pullups(InternalPullup::enabled_22k_ohm);
        Slave1.stop_listening();
        Slave1.set_internal_pullups(InternalPullup::enabled_22k_ohm);
        Slave2.stop_listening();
        Slave2.set_internal_pullups(InternalPullup::enabled_22k_ohm);
        Loopback::disable_all_pullups();
        TestSuite::tearDown();
    }

    static uint32_t measure_rise_time(uint8_t line_pin, uint8_t pullup_pin, __attribute__((unused)) const char* msg) {
        Loopback::disable_all_pullups();
        Loopback::enable_pullup(pullup_pin);
        auto result = line_test::LineTester::TestLine(line_pin);
        uint32_t rise_time = result.get_estimated_rise_time().average();
//        Serial.print(msg);result.get_estimated_rise_time().printTo(Serial);
        return rise_time;
    }

    static void scl_pullups_give_desired_rise_times() {
        pullups_give_desired_rise_times(
                E2ETestBase::PIN_SNIFF_SCL,
                Loopback::PIN_SCL_FASTEST,
                Loopback::PIN_SCL_120_ns,
                Loopback::PIN_SCL_300_ns,
                Loopback::PIN_SCL_1000_ns);
    }

    static void sda_pullups_give_desired_rise_times() {
        pullups_give_desired_rise_times(
                E2ETestBase::PIN_SNIFF_SDA,
                Loopback::PIN_SDA_FASTEST,
                Loopback::PIN_SDA_120_ns,
                Loopback::PIN_SDA_300_ns,
                Loopback::PIN_SDA_1000_ns);
    }

    static void pullups_give_desired_rise_times(uint8_t line_pin,
            uint8_t pullup_fastest, uint8_t pullup_120, uint8_t pullup_300, uint8_t pullup_1000) {
        uint32_t frequency = 100'000;

        // Ensure the I2C ports are active so that they add their pin
        // capacitance to the line
        Master.set_internal_pullups(InternalPullup::disabled);
        Master.begin(frequency);
        Slave1.set_internal_pullups(InternalPullup::disabled);
        Slave1.listen(0x20);
        Slave2.set_internal_pullups(InternalPullup::disabled);
        Slave2.listen(0x30);

        uint32_t rise_time;
        // THEN the fastest possible rise time is fast enough for FastMode+ 1MHZ
        rise_time = measure_rise_time(line_pin, pullup_fastest, "Expected rise time of 45 ns: ");
        TEST_ASSERT_UINT32_WITHIN(10, 40, rise_time);

        // THEN we can set the maximum rise time for Fast Mode Plus (1MHz)
        rise_time = measure_rise_time(line_pin, pullup_120, "Expected rise time of 120 ns got: ");
        TEST_ASSERT_UINT32_WITHIN(30, 120, rise_time);

        // THEN we can set the maximum rise time for Fast Mode (400kHz)
        rise_time = measure_rise_time(line_pin, pullup_300, "Expected rise time of 300 ns got: ");
        TEST_ASSERT_UINT32_WITHIN(60, 300, rise_time);

        // THEN we can set the maximum rise time for Standard Mode (100kHz)
        rise_time = measure_rise_time(line_pin, pullup_1000, "Expected rise time of 1000 ns got: ");
        TEST_ASSERT_UINT32_WITHIN(200, 1000, rise_time);

        // Clean up
        Slave2.stop_listening();
        Slave1.stop_listening();
        Master.end();
        Loopback::disable_all_pullups();
    }

    void test() final {
        RUN_TEST(scl_pullups_give_desired_rise_times);
        RUN_TEST(sda_pullups_give_desired_rise_times);
    }

    RiseTimeTest() : TestSuite(__FILE__) {};
};

} // signals
} // loopback
} // e2e
#endif //TEENSY4_I2C_TEST_E2E_RISE_TIMES_H
