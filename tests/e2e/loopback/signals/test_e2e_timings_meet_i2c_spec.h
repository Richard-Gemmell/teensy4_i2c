// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_MASTER_MEETS_I2C_SPECIFICATION_H
#define TEENSY4_I2C_TEST_E2E_MASTER_MEETS_I2C_SPECIFICATION_H

#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"
#include "e2e/loopback/loopback_test_base.h"
#include <imx_rt1060/imx_rt1060_i2c_driver.h>
#include <analysis/i2c_timing_analyser.h>
#include <bus_trace/bus_trace_builder.h>
#include <analysis/i2c_design_parameters.h>

namespace e2e {
namespace loopback {
namespace signals {

// This test confirms that the I2C timings meet the I2C Specification
// under a variety of rise times.
// It's not intended to check the details of the design given in
// `documentation/default_i2c_profile.md`. That's covered by
// `test_e2e_timings_meet_design.h`
class TimingsMeetI2CSpecificationTest : public LoopbackTestBase {
public:
    const bool FAST_SDA = true;
    const bool SLOW_SDA = false;
    const bool FAST_SCL = true;
    const bool SLOW_SCL = false;
    static I2CMaster* master;
    static I2CSlave* slave;
    static uint32_t frequency;
    static bool fast_sda_rise_time;
    static bool fast_scl_rise_time;
    static uint32_t sda_rise_time;  // Estimated rise time based on pullups
    static uint32_t scl_rise_time;
    static common::i2c_specification::I2CParameters parameters;

    void setUp() override {
        LoopbackTestBase::setUp();
        master->set_internal_pullups(InternalPullup::disabled);
        slave->set_internal_pullups(InternalPullup::disabled);

        if(fast_sda_rise_time) {
            Loopback::enable_pullup(Loopback::PIN_SDA_FASTEST);
            sda_rise_time = Loopback::SDA_FASTEST_RISE_TIME;
        } else {
            if (frequency == 100'000) {
                Loopback::enable_pullup(Loopback::PIN_SDA_1000_ns);
                sda_rise_time = Loopback::SDA_1000_RISE_TIME;
//                Loopback::enable_pullup(Loopback::PIN_SDA_300_ns);
//                sda_rise_time = Loopback::SDA_300_RISE_TIME;
//                Loopback::enable_pullup(Loopback::PIN_SDA_120_ns);
//                sda_rise_time = Loopback::SDA_120_RISE_TIME;
//                Loopback::enable_pullup(Loopback::PIN_SDA_FASTEST);
//                sda_rise_time = Loopback::SDA_FASTEST_RISE_TIME;
            } else if (frequency == 400'000) {
                Loopback::enable_pullup(Loopback::PIN_SDA_300_ns);
                sda_rise_time = Loopback::SDA_300_RISE_TIME;
            } else if (frequency == 1'000'000) {
                Loopback::enable_pullup(Loopback::PIN_SDA_120_ns);
                sda_rise_time = Loopback::SDA_120_RISE_TIME;
            }
        }
        if(fast_scl_rise_time) {
            Loopback::enable_pullup(Loopback::PIN_SCL_FASTEST);
            scl_rise_time = Loopback::SCL_FASTEST_RISE_TIME;
        } else {
            if (frequency == 100'000) {
                Loopback::enable_pullup(Loopback::PIN_SCL_1000_ns);
                scl_rise_time = Loopback::SCL_1000_RISE_TIME;
//                Loopback::enable_pullup(Loopback::PIN_SCL_300_ns);
//                scl_rise_time = Loopback::SCL_300_RISE_TIME;
//                Loopback::enable_pullup(Loopback::PIN_SCL_120_ns);
//                scl_rise_time = Loopback::SCL_120_RISE_TIME;
//                Loopback::enable_pullup(Loopback::PIN_SCL_FASTEST);
//                scl_rise_time = Loopback::SCL_FASTEST_RISE_TIME;
            } else if (frequency == 400'000) {
                Loopback::enable_pullup(Loopback::PIN_SCL_300_ns);
                scl_rise_time = Loopback::SCL_300_RISE_TIME;
            } else if (frequency == 1'000'000) {
                Loopback::enable_pullup(Loopback::PIN_SCL_120_ns);
                scl_rise_time = Loopback::SCL_120_RISE_TIME;
            }
        }
    }

    static analysis::I2CTimingAnalysis analyse_read_transaction() {
        return E2ETestBase::analyse_read_transaction(master, slave, frequency, sda_rise_time, scl_rise_time);
    }

    static analysis::I2CTimingAnalysis analyse_write_transaction() {
        return E2ETestBase::analyse_write_transaction(master, slave, frequency, sda_rise_time, scl_rise_time);
    }

    static analysis::I2CTimingAnalysis analyse_repeated_read_transaction(bool stop_after_first_read = false) {
        return E2ETestBase::analyse_repeated_read_transaction(master, slave, frequency, sda_rise_time, scl_rise_time, stop_after_first_read);
    }

    // Checks tHD;STA - the startup hold time after a normal start bit
    static void start_hold_time() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the startup hold time (tHD;STA) meets the I2C Specification
        auto start_hold_time = analysis.start_hold_time;
        log_value("Start hold (tHD;STA)", parameters.times.start_hold_time, start_hold_time);
        TEST_ASSERT_TRUE(start_hold_time.meets_specification(parameters.times.start_hold_time));
    }

    // Checks tSU;STA - the startup hold time after a repeated start bit
    static void setup_time_for_repeated_start() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_repeated_read_transaction();

        // THEN the setup time for repeated start (tSU;STA) meets the I2C Specification
        auto start_setup_time = analysis.start_setup_time;
        log_value("Setup time for repeated start (tSU;STA)", parameters.times.start_setup_time, start_setup_time);
        TEST_ASSERT_TRUE(start_setup_time.meets_specification(parameters.times.start_setup_time));
    }

    // Checks tSU;STO - the setup time for STOP condition
    static void stop_setup_time() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the startup hold time (tHD;STA) meets the I2C Specification
        auto stop_setup_time = analysis.stop_setup_time;    // This is the actual time at which the Teensy detected the edges at approx 0.5 Vdd.
        log_value("Stop setup time (tSU;STO)", parameters.times.stop_setup_time, stop_setup_time);
        TEST_ASSERT_TRUE(stop_setup_time.meets_specification(parameters.times.stop_setup_time));
    }

    // Checks tHIGH - the HIGH period of the SCL clock
    static void clock_high_time() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the SCL HIGH time meets the specification
        auto clock_high = analysis.scl_high_time;
        log_value("HIGH period of SCL clock (tHIGH)", parameters.times.scl_high_time, clock_high);
        TEST_ASSERT_TRUE(clock_high.meets_specification(parameters.times.scl_high_time));
    }

    // Checks tLOW - the LOW period of the SCL clock
    static void clock_low_time() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the SCL LOW time meets the specification
        auto low_time = analysis.scl_low_time;
        log_value("LOW period of SCL clock (tLOW)", parameters.times.scl_low_time, low_time);
        TEST_ASSERT_TRUE(low_time.meets_specification(parameters.times.scl_low_time));
    }

    // Shows that the Master sets the clock speed correctly.
    // Actually checks the period of each SCL cycle
    static void master_clock_frequency() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the clock frequency (fSCL) meets the I2C Specification
        auto clock_frequency = analysis.clock_frequency;
        log_value("SCL clock frequency (fSCL). I2C spec", parameters.times.frequency, clock_frequency);
        auto expected = parameters.times.frequency;
        if(frequency == 100'000) {
            // Allow the odd cycle to be very slightly too fast.
            expected = common::i2c_specification::TimeRange{.min = 0, .max = 100'050};
        }
        if(frequency == 1'000'000) {
            // Allow the odd cycle to be very slightly too fast.
            expected = common::i2c_specification::TimeRange{.min = 0, .max = 1'004'000};
        }
        TEST_ASSERT_TRUE(clock_frequency.meets_specification(expected));
    }

    // Checks tBUF - bus free time between a STOP and START condition
    static void bus_free_time() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_repeated_read_transaction(true);

        // THEN the bus free time meets the I2C Specification
        auto bus_free_time = analysis.bus_free_time;
        log_value("Bus free time (tBUF)", parameters.times.bus_free_time, bus_free_time);
        TEST_ASSERT_TRUE(bus_free_time.meets_specification(parameters.times.bus_free_time));
    }

    // Checks tSU;DAT - data setup time
    static void setup_data_time_read() {
        // Check tSU;DAT when the slave controls SDA to send data to the master
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the setup data time meets the I2C Specification
        auto data_setup_time = analysis.data_setup_time;
        log_value("Data setup time (tSU;DAT)", parameters.times.data_setup_time, data_setup_time);
        TEST_ASSERT_TRUE(data_setup_time.meets_specification(parameters.times.data_setup_time));
    }

    // Checks tSU;DAT - data setup time
    static void setup_data_time_write() {
        // Check tSU;DAT when the master has full control of SDA
        // WHEN the master writes to the slave
        auto analysis = analyse_write_transaction();

        // THEN the setup data time meets the I2C Specification
        auto data_setup_time = analysis.data_setup_time;
        log_value("Data setup time (tSU;DAT)", parameters.times.data_setup_time, data_setup_time);
        TEST_ASSERT_TRUE(data_setup_time.meets_specification(parameters.times.data_setup_time));
    }

    // Checks tHD;DAT - data setup time
    static void data_hold_time_read() {
        // Check tHD;DAT when the slave controls SDA to send data to the master
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the data hold time meets the I2C Specification
        auto data_hold_time = analysis.data_hold_time;
        log_value("Data hold time (tHD;DAT)", parameters.times.data_hold_time, data_hold_time);
        TEST_ASSERT_TRUE(data_hold_time.meets_specification(parameters.times.data_hold_time));
    }

    // Checks tHD;DAT - data setup time
    static void data_hold_time_write() {
        // Check tHD;DAT when the master has full control of SDA
        // WHEN the master writes to the slave
        auto analysis = analyse_write_transaction();

        // THEN the data hold time meets the I2C Specification
        auto data_hold_time = analysis.data_hold_time;
        log_value("Data hold time (tHD;DAT)", parameters.times.data_hold_time, data_hold_time);
        TEST_ASSERT_TRUE(data_hold_time.meets_specification(parameters.times.data_hold_time));
    }

    // Checks tVD;DAT - data valid time
    static void data_valid_time_read() {
        // Check tVD;DAT when the slave controls SDA to send data to the master
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the data valid time meets the I2C Specification
        auto actual = analysis.data_valid_time;
        log_value("Data valid time (tVD;DAT)", parameters.times.data_valid_time, actual);
        TEST_ASSERT_TRUE(actual.meets_specification(parameters.times.data_valid_time));
    }

    // Checks tVD;DAT - data valid time
    static void data_valid_time_write() {
        // Check tVD;DAT when the master has full control of SDA
        // WHEN the master writes to the slave
        auto analysis = analyse_write_transaction();

        // THEN the data valid time meets the I2C Specification
        auto actual = analysis.data_valid_time;
        log_value("Data valid time (tVD;DAT)", parameters.times.data_valid_time, actual);
        TEST_ASSERT_TRUE(actual.meets_specification(parameters.times.data_valid_time));
    }

    static void test_suite(const char* message, bool fast_sda, bool fast_scl) {
        Serial.println(message);
        master = &Master;
        slave = &Slave1;
        fast_sda_rise_time = fast_sda;
        fast_scl_rise_time = fast_scl;
        RUN_TEST(start_hold_time);
        RUN_TEST(setup_time_for_repeated_start);
        RUN_TEST(stop_setup_time);
        RUN_TEST(clock_high_time);
        RUN_TEST(clock_low_time);
        RUN_TEST(master_clock_frequency);
        RUN_TEST(bus_free_time);
        RUN_TEST(setup_data_time_read);
        RUN_TEST(setup_data_time_write);
        RUN_TEST(data_hold_time_read);
        RUN_TEST(data_hold_time_write);
        RUN_TEST(data_valid_time_read);
        RUN_TEST(data_valid_time_write);
    }

    static void log_value(const char* msg, common::i2c_specification::TimeRange expected, const analysis::DurationStatistics& actual) {
        return;
        if(expected.max == UINT32_MAX) {
            Serial.printf("%s. Expected %u+. Actual ", msg, expected.min, expected.max);
        } else {
            Serial.printf("%s. Expected %u-%u. Actual ", msg, expected.min, expected.max);
        }
        Serial.println(actual);
    }

    void test() final {
        // Tests timing of each supported frequency.
        // The different rise times trigger different edge cases.
        frequency = 100'000;
        parameters = common::i2c_specification::StandardMode;
        test_suite("100 kHz - Fast Rise Times", FAST_SDA, FAST_SCL);
        test_suite("100 kHz - Fast SDA, Slow SCL", FAST_SDA, SLOW_SCL);
        test_suite("100 kHz - Slow SDA, Fast SCL", SLOW_SDA, FAST_SCL);
        test_suite("100 kHz - Slow Rise Times", SLOW_SDA, SLOW_SCL);
        Serial.println(".");

        frequency = 400'000;
        parameters = common::i2c_specification::FastMode;
        test_suite("400 kHz - Fast Rise Times", FAST_SDA, FAST_SCL);
        test_suite("400 kHz - Fast SDA, Slow SCL", FAST_SDA, SLOW_SCL);
        test_suite("400 kHz - Slow SDA, Fast SCL", SLOW_SDA, FAST_SCL);
        test_suite("400 kHz - Slow Rise Times", SLOW_SDA, SLOW_SCL);
        Serial.println(".");

        frequency = 1'000'000;
        parameters = common::i2c_specification::FastModePlus;
        test_suite("1 MHz - Fast Rise Times", FAST_SDA, FAST_SCL);
        test_suite("1 MHz - Fast SDA, Slow SCL", FAST_SDA, SLOW_SCL);
        test_suite("1 MHz - Slow SDA, Fast SCL", SLOW_SDA, FAST_SCL);
        test_suite("1 MHz - Slow Rise Times", SLOW_SDA, SLOW_SCL);
    }

    TimingsMeetI2CSpecificationTest() : LoopbackTestBase(__FILE__) {};
};

// Define statics
I2CMaster* e2e::loopback::signals::TimingsMeetI2CSpecificationTest::master;
I2CSlave* e2e::loopback::signals::TimingsMeetI2CSpecificationTest::slave;
uint32_t e2e::loopback::signals::TimingsMeetI2CSpecificationTest::frequency;
bool e2e::loopback::signals::TimingsMeetI2CSpecificationTest::fast_sda_rise_time;
bool e2e::loopback::signals::TimingsMeetI2CSpecificationTest::fast_scl_rise_time;
uint32_t e2e::loopback::signals::TimingsMeetI2CSpecificationTest::sda_rise_time;
uint32_t e2e::loopback::signals::TimingsMeetI2CSpecificationTest::scl_rise_time;
common::i2c_specification::I2CParameters e2e::loopback::signals::TimingsMeetI2CSpecificationTest::parameters;

} // signals
} // loopback
} // e2e
#endif //TEENSY4_I2C_TEST_E2E_MASTER_MEETS_I2C_SPECIFICATION_H
