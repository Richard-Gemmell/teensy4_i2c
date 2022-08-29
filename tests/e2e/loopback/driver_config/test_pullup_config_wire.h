// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_PULLUP_CONFIG_WIRE_H
#define TEENSY4_I2C_TEST_PULLUP_CONFIG_WIRE_H

#include <unity.h>
#include <Arduino.h>
#include <line_test/line_tester.h>
#include "utils/test_suite.h"
#include "e2e/loopback/loopback.h"
#include <i2c_driver_wire.h>

namespace e2e {
namespace loopback {
namespace driver_config {

// Proves that developers can disable or select the internal
// pullup resistors with the Wire library.
class PullupConfigWireTest : public TestSuite {
public:
    void setUp() override {
        TestSuite::setUp();
        Loopback::disable_all_pullups();
    }

    void tearDown() override {
        TestSuite::tearDown();
    }

    static uint32_t measure_rise_time(uint8_t line_pin, __attribute__((unused)) const char* msg) {
        Loopback::disable_all_pullups();
        Wire.begin();
        Wire1.begin(0x20);
        delayMicroseconds(2);

        auto result = line_test::LineTester::TestLine(line_pin, 5'000);
        uint32_t rise_time = result.get_estimated_rise_time().min();
//        Serial.print(msg);result.get_estimated_rise_time().printTo(Serial);

        Wire.end();
        Wire1.end();
        return rise_time;
    }

    static void can_set_internal_pullups_for_scl() {
        pullups_give_expected_rise_times(Loopback::PIN_SNIFF_SCL);
    }

    static void can_set_internal_pullups_for_sda() {
        pullups_give_expected_rise_times(Loopback::PIN_SNIFF_SDA);
    }

    static void pullups_give_expected_rise_times(uint8_t line_pin) {
        uint32_t rise_time;

        // WHEN we disable pullups
        Wire.setInternalPullups(InternalPullup::disabled);
        Wire1.setInternalPullups(InternalPullup::disabled);
        // THEN the line doesn't rise at all
        rise_time = measure_rise_time(line_pin, "Expected test to time out: ");
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, rise_time);

        // WHEN we enable 22k pullups
        Wire.setInternalPullups(InternalPullup::enabled_22k_ohm);
        Wire1.setInternalPullups(InternalPullup::enabled_22k_ohm);
        // THEN the rise should be fairly short
        rise_time = measure_rise_time(line_pin, "22k rise time: ");
        TEST_ASSERT_UINT32_WITHIN(150, 670, rise_time);

        // WHEN we enable 47k pullups
        Wire.setInternalPullups(InternalPullup::enabled_47k_ohm);
        Wire1.setInternalPullups(InternalPullup::enabled_47k_ohm);
        // THEN the rise should be medium
        rise_time = measure_rise_time(line_pin, "47k rise time: ");
        TEST_ASSERT_UINT32_WITHIN(280, 1400, rise_time);

        // WHEN we enable 100k pullups
        Wire.setInternalPullups(InternalPullup::enabled_100k_ohm);
        Wire1.setInternalPullups(InternalPullup::enabled_100k_ohm);
        // THEN the rise should be very long
        rise_time = measure_rise_time(line_pin, "100k rise time: ");
        TEST_ASSERT_UINT32_WITHIN(600, 3000, rise_time);
    }

    void test() final {
        RUN_TEST(can_set_internal_pullups_for_scl);
        RUN_TEST(can_set_internal_pullups_for_sda);
    }

    PullupConfigWireTest() : TestSuite(__FILE__) {};
};

}
}
}

#endif //TEENSY4_I2C_TEST_PULLUP_CONFIG_WIRE_H
