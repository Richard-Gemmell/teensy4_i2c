// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_PULLUP_CONFIG_H
#define TEENSY4_I2C_TEST_PULLUP_CONFIG_H

#include <unity.h>
#include <Arduino.h>
#include <line_test/line_tester.h>
#include "utils/test_suite.h"
#include "e2e/loopback/loopback.h"

namespace e2e {
namespace loopback {
namespace driver_config {

// Proves that developers can disable or select the internal pullup resistors.
class PullupConfigTest : public TestSuite {
public:
    void setUp() override {
        TestSuite::setUp();
        Loopback::disable_all_pullups();
    }

    void tearDown() override {
        Master.set_internal_pullups(InternalPullup::enabled_22k_ohm);
        Slave1.set_internal_pullups(InternalPullup::enabled_22k_ohm);
        Slave2.set_internal_pullups(InternalPullup::enabled_22k_ohm);
        TestSuite::tearDown();
    }

    static uint32_t measure_rise_time(uint8_t line_pin, __attribute__((unused)) const char* msg) {
        Loopback::disable_all_pullups();
        Master.begin(100'000);
        Slave1.listen(0x20);
        Slave2.listen(0x22);
        delayMicroseconds(2);

        auto result = line_test::LineTester::TestLine(line_pin, 5'000);
        uint32_t rise_time = result.get_estimated_rise_time().min();
//        Serial.print(msg);result.get_estimated_rise_time().printTo(Serial);

        Master.end();
        Slave1.stop_listening();
        return rise_time;
    }

    static void can_set_internal_pullups_for_scl() {
        pullups_give_expected_rise_times(E2ETestBase::PIN_SNIFF_SCL);
    }

    static void can_set_internal_pullups_for_sda() {
        pullups_give_expected_rise_times(E2ETestBase::PIN_SNIFF_SDA);
    }

    static void pullups_give_expected_rise_times(uint8_t line_pin) {
        // WHEN we disable pullups
        Master.set_internal_pullups(InternalPullup::disabled);
        Slave1.set_internal_pullups(InternalPullup::disabled);
        Slave2.set_internal_pullups(InternalPullup::disabled);

        // THEN the line doesn't rise at all
        auto no_pullup_time = measure_rise_time(line_pin, "Expected test to time out: ");
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, no_pullup_time);

        // WHEN we compare the rise times for 22k, 47k and 100k
        Master.set_internal_pullups(InternalPullup::enabled_22k_ohm);
        Slave1.set_internal_pullups(InternalPullup::enabled_22k_ohm);
        Slave2.set_internal_pullups(InternalPullup::enabled_22k_ohm);
        auto _22k_rise_time = measure_rise_time(line_pin, "22k rise time: ");

        Master.set_internal_pullups(InternalPullup::enabled_47k_ohm);
        Slave1.set_internal_pullups(InternalPullup::enabled_47k_ohm);
        Slave2.set_internal_pullups(InternalPullup::enabled_47k_ohm);
        auto _47k_rise_time = measure_rise_time(line_pin, "47k rise time: ");

        Master.set_internal_pullups(InternalPullup::enabled_100k_ohm);
        Slave1.set_internal_pullups(InternalPullup::enabled_100k_ohm);
        Slave2.set_internal_pullups(InternalPullup::enabled_100k_ohm);
        auto _100k_rise_time = measure_rise_time(line_pin, "100k rise time: ");

        // THEN the 22k rise time should be a reasonable number
        // (The exact value depends on the board layout and how
        // many I2C ports are connected.)
        TEST_ASSERT_UINT32_WITHIN(700, 1000, _22k_rise_time);

        // THEN the 47k rise time should be 47/22 times longer than the 22k time
        auto expected_47k_time = (uint32_t)(_22k_rise_time*(47.0/22.0));
        TEST_ASSERT_UINT32_WITHIN(expected_47k_time/5, expected_47k_time, _47k_rise_time);

        // AND the 100k rise time should be 100/47 times longer than the 47k time
        auto expected_100k_time = (uint32_t)(_47k_rise_time*(100.0/47.0));
        TEST_ASSERT_UINT32_WITHIN(expected_100k_time/5, expected_100k_time, _100k_rise_time);
    }

    void test() final {
        RUN_TEST(can_set_internal_pullups_for_scl);
        RUN_TEST(can_set_internal_pullups_for_sda);
    }

    PullupConfigTest() : TestSuite(__FILE__) {};
};

}
}
}

#endif //TEENSY4_I2C_TEST_PULLUP_CONFIG_H
