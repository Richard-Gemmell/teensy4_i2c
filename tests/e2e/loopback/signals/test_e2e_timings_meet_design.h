// Copyright (c) 2023 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_TIMINGS_MEET_DESIGN_H
#define TEENSY4_I2C_TEST_E2E_TIMINGS_MEET_DESIGN_H

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

// This test confirms that the nominal I2C timings match the design
// in `documentation/default_i2c_profile.md`.
// In particular, it confirms that any changes to the timings remain
// within limits.
// It's a fairly crude test. The Python tests in
// `tools/i2c_timing_calculator/profile_test` are finer grained.
class TimingsMeetDesignTest : public LoopbackTestBase {
public:
    static I2CMaster* master;
    static I2CSlave* slave;
    static uint32_t frequency;
    static uint32_t sda_rise_time;  // Estimated rise time based on pullups
    static uint32_t scl_rise_time;
    static common::i2c_specification::I2CParameters parameters;

    const common::i2c_specification::I2CParameters StandardModeDesignParameters = {
        .times = {
            .output_fall_time = {.min = 0, .max = 0},
            .spike_width = {.min = 0, .max = 0},
            .frequency = {.min = 99'950, .max = 100'050},
            .start_hold_time = {.min = 5'950, .max = 6'050},
            .scl_low_time = {.min = 5'030, .max = 5'130},
            .scl_high_time = {.min = 4'840, .max = 4'940},
            .start_setup_time = {.min = 6'175, .max = 6'275},
            .data_hold_time = {.min = 170, .max = 1'100}, // Min determined by Slave. Max determined by Master.
            .data_setup_time = {.min = 3'950, .max = 4'900}, // Min determined by Master. Max determined by Slave.
            .rise_time = {.min = 0, .max = 0},
            .fall_time = {.min = 0, .max = 0},
            .stop_setup_time = {.min = 6'200, .max = 6'275},
            .bus_free_time = {.min = 7'250, .max = 7'480},
            .data_valid_time = {.min = 0, .max = 0},
        }
    };

    const common::i2c_specification::I2CParameters FastModeDesignParameters = {
        .times = {
            .output_fall_time = {.min = 0, .max = 0},
            .spike_width = {.min = 0, .max = 0},
            .frequency = {.min = 393'000, .max = 400'000},
            .start_hold_time = {.min = 700, .max = 750},
            .scl_low_time = {.min = 1390, .max = 1470},
            .scl_high_time = {.min = 1040, .max = 1120},
            .start_setup_time = {.min = 950, .max = 1025},
            .data_hold_time = {.min = 170, .max = 420}, // Min determined by Slave. Max determined by Master.
            .data_setup_time = {.min = 980, .max = 1'250}, // Min determined by Master. Max determined by Slave.
            .rise_time = {.min = 0, .max = 0},
            .fall_time = {.min = 0, .max = 0},
            .stop_setup_time = {.min = 975, .max = 1025},
            .bus_free_time = {.min = 2'350, .max = 2'500},
            .data_valid_time = {.min = 0, .max = 0},
        }
    };

    const common::i2c_specification::I2CParameters FastModePlusDesignParameters = {
        .times = {
            .output_fall_time = {.min = 0, .max = 0},
            .spike_width = {.min = 0, .max = 0},
            .frequency = {.min = 965'000, .max = 1'004'000},
            .start_hold_time = {.min = 300, .max = 350},
            .scl_low_time = {.min = 580, .max = 660},
            .scl_high_time = {.min = 340, .max = 400},
            .start_setup_time = {.min = 425, .max = 475},
            .data_hold_time = {.min = 170, .max = 240}, // Min determined by Slave. Max determined by Master.
            .data_setup_time = {.min = 350, .max = 450}, // Min determined by Master. Max determined by Slave.
            .rise_time = {.min = 0, .max = 0},
            .fall_time = {.min = 0, .max = 0},
            .stop_setup_time = {.min = 425, .max = 475},
            .bus_free_time = {.min = 1'500, .max = 1'650},
            .data_valid_time = {.min = 0, .max = 0},
        }
    };

    void setUp() override {
        LoopbackTestBase::setUp();
        master->set_internal_pullups(InternalPullup::disabled);
        slave->set_internal_pullups(InternalPullup::disabled);
        Loopback::enable_pullup(Loopback::PIN_SDA_FASTEST);
        sda_rise_time = Loopback::SDA_FASTEST_RISE_TIME;
        Loopback::enable_pullup(Loopback::PIN_SCL_FASTEST);
        scl_rise_time = Loopback::SCL_FASTEST_RISE_TIME;
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
        log_value("SCL clock frequency (fSCL).", parameters.times.frequency, clock_frequency);
        TEST_ASSERT_TRUE(clock_frequency.meets_specification(parameters.times.frequency));
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

    // Checks tHD;DAT - data hold time
    static void data_hold_time_read() {
        // Check tHD;DAT when the slave controls SDA to send data to the master
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the data hold time meets the I2C Specification
        auto data_hold_time = analysis.data_hold_time;
        log_value("Data hold time (tHD;DAT)", parameters.times.data_hold_time, data_hold_time);
        TEST_ASSERT_TRUE(data_hold_time.meets_specification(parameters.times.data_hold_time));
    }

    // Checks tHD;DAT - data hold time
    static void data_hold_time_write() {
        // Check tHD;DAT when the master has full control of SDA
        // WHEN the master writes to the slave
        auto analysis = analyse_write_transaction();

        // THEN the data hold time meets the I2C Specification
        auto data_hold_time = analysis.data_hold_time;
        log_value("Data hold time (tHD;DAT)", parameters.times.data_hold_time, data_hold_time);
        TEST_ASSERT_TRUE(data_hold_time.meets_specification(parameters.times.data_hold_time));
    }

    // Checks tSU;DAT - data setup time
    // WARNING: The setup data time is a side effect of the data hold time and the SCL low time.
    // It's tested here just to make sure nothing changed by accident.
    // Don't bother testing data valid time (tVD;DAT) as it's even more derivative.
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
    // WARNING: The setup data time is a side effect of the data hold time and the SCL low time.
    // It's tested here just to make sure nothing changed by accident.
    // Don't bother testing data valid time (tVD;DAT) as it's even more derivative.
    static void setup_data_time_write() {
        // Check tSU;DAT when the master has full control of SDA
        // WHEN the master writes to the slave
        auto analysis = analyse_write_transaction();

        // THEN the setup data time meets the I2C Specification
        auto data_setup_time = analysis.data_setup_time;
        log_value("Data setup time (tSU;DAT)", parameters.times.data_setup_time, data_setup_time);
        TEST_ASSERT_TRUE(data_setup_time.meets_specification(parameters.times.data_setup_time));
    }

    static void test_suite(const char* message) {
        Serial.println(message);
        master = &Master;
        slave = &Slave1;
        RUN_TEST(start_hold_time);
        RUN_TEST(setup_time_for_repeated_start);
        RUN_TEST(stop_setup_time);
        RUN_TEST(clock_high_time);
        RUN_TEST(clock_low_time);
        RUN_TEST(master_clock_frequency);
        RUN_TEST(bus_free_time);
        RUN_TEST(data_hold_time_read);
        RUN_TEST(data_hold_time_write);
        RUN_TEST(setup_data_time_read);
        RUN_TEST(setup_data_time_write);
    }

    static void log_value(const char* msg, common::i2c_specification::TimeRange expected, const analysis::DurationStatistics& actual) {
        return;
        if(expected.max == UINT32_MAX) {
            Serial.printf("%s. Expected %u+. Actual ", msg, expected.min, expected.max);
        } else {
            Serial.printf("%s. Expected %u-%u. Actual ", msg, expected.min, expected.max);
        }
        Serial.print(actual);
    }

    void test() final {
        // Tests timing of each supported frequency.
        frequency = 100'000;
        parameters = StandardModeDesignParameters;
        test_suite("100 kHz - Fast Rise Times");
        Serial.println(".");

        frequency = 400'000;
        parameters = FastModeDesignParameters;
        test_suite("400 kHz - Fast Rise Times");
        Serial.println(".");

        frequency = 1'000'000;
        parameters = FastModePlusDesignParameters;
        test_suite("1 MHz - Fast Rise Times");
    }

    TimingsMeetDesignTest() : LoopbackTestBase(__FILE__) {};
};

// Define statics
I2CMaster* e2e::loopback::signals::TimingsMeetDesignTest::master;
I2CSlave* e2e::loopback::signals::TimingsMeetDesignTest::slave;
uint32_t e2e::loopback::signals::TimingsMeetDesignTest::frequency;
uint32_t e2e::loopback::signals::TimingsMeetDesignTest::sda_rise_time;
uint32_t e2e::loopback::signals::TimingsMeetDesignTest::scl_rise_time;
common::i2c_specification::I2CParameters e2e::loopback::signals::TimingsMeetDesignTest::parameters;

} // signals
} // loopback
} // e2e

#endif //TEENSY4_I2C_TEST_E2E_TIMINGS_MEET_DESIGN_H
