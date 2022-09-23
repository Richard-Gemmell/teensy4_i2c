// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_MASTER_SIGNALS_H
#define TEENSY4_I2C_TEST_E2E_MASTER_SIGNALS_H

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


const analysis::MasterDesignParameters StandardModeDesignParameters = {
        {100'000, 100'000}, // clock_frequency - maximum allowed
        {5'000, 5'350}, // start_hold_time - max includes measurement error allowance
        {1, 1}, // scl_low_time  // TODO: What values to use for tHIGH and tLOW
        {1, 1}, // scl_high_time
        {5'875, 7'000}, // start_setup_time
        {1, 1}, // data_hold_time
        {1, 1}, // data_setup_time
        {5'000, 6'500}, // stop_setup_time
        {1, 1}, // bus_free_time
};

const analysis::MasterDesignParameters FastModeDesignParameters = {
        {400'000, 400'000}, // clock_frequency - maximum allowed
        {750, 1'095}, // start_hold_time - max includes measurement error allowance
        {1, 1}, // scl_low_time
        {1, 1}, // scl_high_time
        {750, 1'095}, // start_setup_time
        {1, 1}, // data_hold_time
        {1, 1}, // data_setup_time
        {750, 1'400}, // stop_setup_time
        {1, 1}, // bus_free_time
};

const analysis::MasterDesignParameters FastModePlusDesignParameters = {
        {1'000'000, 1'000'000}, // clock_frequency - maximum allowed
        {325, 465}, // start_hold_time - min allowed by spec +5% rounded up i.e. >(260*1.05)
        {1, 1}, // scl_low_time
        {1, 1}, // scl_high_time
        {325, 465}, // start_setup_time
        {1, 1}, // data_hold_time
        {1, 1}, // data_setup_time
        {380, 600}, // stop_setup_time
        {1, 1}, // bus_free_time
};

// Tests the most common scenarios.
class MasterSignalsTest : public LoopbackTestBase {
public:
    const bool FAST_SDA = true;
    const bool SLOW_SDA = false;
    const bool FAST_SCL = true;
    const bool SLOW_SCL = false;
    // These 2 byte values contain all four bit transitions 0->0, 0->1, 1->1 and 1->0
    // The first and last bit of each byte affect the edges for ACKs
    // so one byte is the inverse of the other.
    const static uint8_t BYTE_A = 0x58; // 0101 1000
    const static uint8_t BYTE_B = 0xA7; // 1010 0111
    const static uint8_t ADDRESS = 0x53;
    static I2CMaster* master;
    static I2CSlave* slave;
    static uint32_t frequency;
    static bool fast_sda_rise_time;
    static bool fast_scl_rise_time;
    static uint32_t sda_rise_time;  // Estimated rise time based on pullups
    static uint32_t scl_rise_time;
    static common::i2c_specification::I2CParameters parameters;
    static analysis::MasterDesignParameters master_design_parameters;

    static void print_trace(const bus_trace::BusTrace& trace) {
        Serial.println(trace);
//        for (size_t i = 0; i < trace.event_count(); ++i) {
//            Serial.printf("Index %d: delta %d ns\n", i, common::hal::TeensyTimestamp::ticks_to_nanos(trace.event(i)->delta_t_in_ticks));
//        }
    }

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
            } else if (frequency == 400'000) {
                Loopback::enable_pullup(Loopback::PIN_SCL_300_ns);
                scl_rise_time = Loopback::SCL_300_RISE_TIME;
            } else if (frequency == 1'000'000) {
                Loopback::enable_pullup(Loopback::PIN_SCL_120_ns);
                scl_rise_time = Loopback::SCL_120_RISE_TIME;
            }
        }
        // Rise times are longer if the oscilloscope is attached
//        sda_rise_time = (uint32_t)(sda_rise_time * 1.2);
//        scl_rise_time = (uint32_t)(scl_rise_time * 1.2);
    }

    static analysis::I2CTimingAnalysis analyse_read_transaction() {
        bus_trace::BusTrace trace(&clock, MAX_EVENTS);

        const uint8_t tx_buffer[] = {BYTE_A, BYTE_B};
        slave->set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
        uint8_t rx_buffer[] = {0x00, 0x00};

        // Master reads from slave
        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [&rx_buffer](){
            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
// HACK to trigger a restart and bus idle behaviours
//            finish(*master);
//            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
        });
//        print_trace(trace);
        // Ensure the read succeeded
        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));

        return analysis::I2CTimingAnalyser::analyse(trace, sda_rise_time, scl_rise_time);
    }

    static analysis::I2CTimingAnalysis analyse_repeated_read_transaction() {
        bus_trace::BusTrace trace(&clock, MAX_EVENTS);

        const uint8_t tx_buffer[] = {BYTE_A, BYTE_B};
        slave->set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
        uint8_t rx_buffer[] = {0x00, 0x00};

        // Master reads from slave
        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [&rx_buffer](){
            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), false);
            finish(*master);
            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
        });
//        print_trace(trace);
        // Ensure the read succeeded
        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));

        return analysis::I2CTimingAnalyser::analyse(trace, sda_rise_time, scl_rise_time);
    }

    // Checks tHD;STA - the startup hold time after a normal start bit
    static void start_hold_time() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the startup hold time (tHD;STA) meets the I2C Specification
        auto start_hold_time = analysis.start_hold_time;
//        log_value("Start hold (tHD;STA)", master_design_parameters.start_hold_time, start_hold_time);
        TEST_ASSERT_TRUE(start_hold_time.meets_specification(parameters.times.start_hold_time));

        // AND the startup hold time fits the design of this driver
        TEST_ASSERT_TRUE(start_hold_time.meets_specification(master_design_parameters.start_hold_time));
    }

    // Checks tSU;STO - the setup time for STOP condition
    static void stop_setup_time() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the startup hold time (tHD;STA) meets the I2C Specification
        auto stop_setup_time = analysis.stop_setup_time;    // This is the actual time at which the Teensy detected the edges at approx 0.5 Vdd.
//        log_value("Stop setup time (tSU;STO)", master_design_parameters.stop_setup_time, stop_setup_time);
        TEST_ASSERT_TRUE(stop_setup_time.meets_specification(parameters.times.stop_setup_time));

        // AND the setup stop time fits the design of this driver
        TEST_ASSERT_TRUE(stop_setup_time.meets_specification(master_design_parameters.stop_setup_time));
    }

    // Checks tSU;STA - the startup hold time after a repeated start bit
    static void setup_time_for_repeated_start() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_repeated_read_transaction();

        // THEN the setup time for repeated start (tSU;STA) meets the I2C Specification
        auto start_setup_time = analysis.start_setup_time;
        log_value("Setup time for repeated start (tSU;STA)", master_design_parameters.start_setup_time, start_setup_time);
        TEST_ASSERT_TRUE(start_setup_time.meets_specification(parameters.times.start_setup_time));

        // AND the setup time for repeated start time fits the design of this driver
        TEST_ASSERT_TRUE(start_setup_time.meets_specification(master_design_parameters.start_setup_time));
    }

    // Checks tHIGH - the HIGH period of the SCL clock
    static void clock_high_time() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the SCL HIGH time meets the specification
        auto clock_high = analysis.scl_high_time;
        log_value("HIGH period of SCL clock (tHIGH)", master_design_parameters.scl_high_time, clock_high);
        TEST_ASSERT_TRUE(clock_high.meets_specification(parameters.times.frequency));

        // AND the SCL HIGH time is at least as long as required
        // We can't check the average because it depends on the rise and fall times
//        TEST_ASSERT_GREATER_OR_EQUAL_UINT32(master_design_parameters.scl_high_time, clock_high.min());
        TEST_FAIL_MESSAGE("What do we expect?");
    }

    // Checks tLOW - the LOW period of the SCL clock
    static void clock_low_time() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the SCL LOW time meets the specification
        auto low_time = analysis.scl_low_time;
        log_value("LOW period of SCL clock (tLOW)", master_design_parameters.scl_low_time, low_time);
        TEST_ASSERT_TRUE(low_time.meets_specification(parameters.times.scl_low_time));

        // AND the SCL LOW time fits the design
        // We can't check the average because it depends on the rise and fall times
//        TEST_ASSERT_GREATER_OR_EQUAL_UINT32(master_design_parameters.scl_low_time, low_time.min());
        TEST_FAIL_MESSAGE("What do we expect?");
    }

    // Shows that the Master sets the clock speed correctly.
    // Actually checks the period of each SCL cycle
    static void master_clock_frequency() {
        // WHEN the master reads data from the slave
        auto analysis = analyse_read_transaction();

        // THEN the clock frequency (fSCL) meets the I2C Specification
        auto clock_frequency = analysis.clock_frequency;
        log_value("SCL clock frequency (fSCL). I2C spec", parameters.times.frequency, clock_frequency);
        TEST_ASSERT_TRUE(clock_frequency.meets_specification(parameters.times.frequency));

        // AND the clock frequency fits the design of this driver
        // We can't check the average because it depends on the rise and fall times
//        log_value("SCL clock frequency (fSCL). Design", master_design_parameters.clock_frequency, clock_frequency);
        TEST_ASSERT_TRUE(clock_frequency.meets_specification(master_design_parameters.clock_frequency));
        TEST_FAIL_MESSAGE("What do we expect?");
    }

    static void test_suite(const char* message, bool fast_sda, bool fast_scl) {
        Serial.println(message);
        master = &Master;
        slave = &Slave1;
        fast_sda_rise_time = fast_sda;
        fast_scl_rise_time = fast_scl;
//        RUN_TEST(start_hold_time);
        RUN_TEST(setup_time_for_repeated_start);
//        RUN_TEST(stop_setup_time);
//        RUN_TEST(clock_high_time);
//        RUN_TEST(clock_low_time);
//        RUN_TEST(master_clock_frequency);
    }

    static void log_value(const char* msg, common::i2c_specification::TimeRange expected, const analysis::DurationStatistics& actual) {
        Serial.printf("%s. Expected %d-%d. Actual ", msg, expected.min, expected.max);
        Serial.println(actual);
    }

    static void log_value(const char* msg, uint32_t expected, const analysis::DurationStatistics& actual) {
        Serial.printf("%s. Expected %d. Actual ", msg, expected);
        Serial.println(actual);
    }

    void test() final {
        // Tests timing of each supported frequency.
        // The different rise times trigger different edge cases.
        frequency = 100'000;
        parameters = common::i2c_specification::StandardMode;
        master_design_parameters = StandardModeDesignParameters;
        test_suite("100 kHz - Fast Rise Times", FAST_SDA, FAST_SCL);
        test_suite("100 kHz - Fast SDA, Slow SCL", FAST_SDA, SLOW_SCL);
        test_suite("100 kHz - Slow SDA, Fast SCL", SLOW_SDA, FAST_SCL);
        test_suite("100 kHz - Slow Rise Times", SLOW_SDA, SLOW_SCL);
        Serial.println(".");

        frequency = 400'000;
        parameters = common::i2c_specification::FastMode;
        master_design_parameters = FastModeDesignParameters;
        test_suite("400 kHz - Fast Rise Times", FAST_SDA, FAST_SCL);
        test_suite("400 kHz - Fast SDA, Slow SCL", FAST_SDA, SLOW_SCL);
        test_suite("400 kHz - Slow SDA, Fast SCL", SLOW_SDA, FAST_SCL);
        test_suite("400 kHz - Slow Rise Times", SLOW_SDA, SLOW_SCL);
        Serial.println(".");

        frequency = 1'000'000;
        parameters = common::i2c_specification::FastModePlus;
        master_design_parameters = FastModePlusDesignParameters;
        test_suite("1 MHz - Fast Rise Times", FAST_SDA, FAST_SCL);
        test_suite("1 MHz - Fast SDA, Slow SCL", FAST_SDA, SLOW_SCL);
        test_suite("1 MHz - Slow SDA, Fast SCL", SLOW_SDA, FAST_SCL);
        test_suite("1 MHz - Slow Rise Times", SLOW_SDA, SLOW_SCL);
    }

    MasterSignalsTest() : LoopbackTestBase(__FILE__) {};
};

// Define statics
I2CMaster* e2e::loopback::signals::MasterSignalsTest::master;
I2CSlave* e2e::loopback::signals::MasterSignalsTest::slave;
uint32_t e2e::loopback::signals::MasterSignalsTest::frequency;
bool e2e::loopback::signals::MasterSignalsTest::fast_sda_rise_time;
bool e2e::loopback::signals::MasterSignalsTest::fast_scl_rise_time;
uint32_t e2e::loopback::signals::MasterSignalsTest::sda_rise_time;
uint32_t e2e::loopback::signals::MasterSignalsTest::scl_rise_time;
common::i2c_specification::I2CParameters e2e::loopback::signals::MasterSignalsTest::parameters;
analysis::MasterDesignParameters e2e::loopback::signals::MasterSignalsTest::master_design_parameters = StandardModeDesignParameters;

// Define statics
//int SlaveSignalTest::value;

} // signals
} // loopback
} // e2e
#endif //TEENSY4_I2C_TEST_E2E_MASTER_SIGNALS_H
