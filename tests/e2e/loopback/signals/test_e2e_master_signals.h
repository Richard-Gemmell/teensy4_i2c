// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_MASTER_SIGNALS_H
#define TEENSY4_I2C_TEST_E2E_MASTER_SIGNALS_H

#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"
#include "e2e/e2e_test_base.h"
#include <imx_rt1060/imx_rt1060_i2c_driver.h>
//#include <analysis/i2c_timing_analyser.h>
#include <bus_trace/bus_trace_builder.h>
#include <analysis/i2c_design_parameters.h>

namespace e2e {
namespace loopback {
namespace signals {

const analysis::MasterDesignParameters StandardModeDesignParameters = {
        100'000, // clock_frequency
        4'250, // start_hold_time - min allowed by spec +5%
        0, // scl_low_time
        0, // scl_high_time
        0, // data_hold_time
        0, // data_setup_time
        0, // stop_setup_time
        0, // bus_free_time
};

const analysis::MasterDesignParameters FastModeDesignParameters = {
        400'000, // clock_frequency
        667, // start_hold_time - min allowed by spec +5% rounded up i.e. >(600*1.05)
        0, // scl_low_time
        0, // scl_high_time
        0, // data_hold_time
        0, // data_setup_time
        0, // stop_setup_time
        0, // bus_free_time
};

const analysis::MasterDesignParameters FastModePlusDesignParameters = {
        1'000'000, // clock_frequency
        292, // start_hold_time - min allowed by spec +5% rounded up i.e. >(260*1.05)
        0, // scl_low_time
        0, // scl_high_time
        0, // data_hold_time
        0, // data_setup_time
        0, // stop_setup_time
        0, // bus_free_time
};

// Tests the most common scenarios.
class MasterSignalsTest : public E2ETestBase {
public:
    // These 2 byte values contain all four bit transitions 0->0, 0->1, 1->1 and 1->0
    // The first and last bit of each byte affect the edges for ACKs
    // so one byte is the inverse of the other.
    const static uint8_t BYTE_A = 0x58; // 0101 1000
    const static uint8_t BYTE_B = 0xA7; // 1010 0111
    const static uint8_t ADDRESS = 0x53;
    static I2CMaster* master;
    static I2CSlave* slave;
    static uint32_t frequency;
    static common::i2c_specification::I2CParameters parameters;
    static analysis::MasterDesignParameters master_design_parameters;

#define PIN_TEST 14

//    static void start_bit() {
//        // Checks tHD;STA - the startup hold time
//        bus_trace::BusTrace trace(MAX_EVENTS);
//        const uint8_t tx_buffer[] = {BYTE_A, BYTE_B};
//        slave->set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
//        uint8_t rx_buffer[] = {0x00, 0x00};
//
//        // WHEN we analyse a trace
//        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [&rx_buffer](){
//            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
//// HACK to trigger a restart and bus idle behaviours
////            finish(*master);
////            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
//        });
//        auto analysis = analysis::I2CTimingAnalyser::analyse(trace);
//
//        // THEN the I2C transaction succeeded
//        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));
//        // AND the startup hold time is valid
//        auto start_hold_time = analysis.start_hold_time;
////        log_value("Start hold (tHD;STA)", master_design_parameters.start_hold_time, start_hold_time.average());
//        TEST_ASSERT_TRUE(start_hold_time.meets_specification(parameters.times.start_hold_time));
//        // TODO: Can we standardise the design check?
//        size_t epsilon = 5;    // Expected measurement error == 3 clock ticks
//        TEST_ASSERT_size_t_WITHIN(epsilon, master_design_parameters.start_hold_time, start_hold_time.average());
//    }

//    static void master_clock_speed() {
//        // Shows that the Master sets the clock speed correctly.
//        // Checks that SCL clock LOW and SCL clock HIGH periods
//        // are within spec and as designed.
//
//        bus_trace::BusTrace trace(MAX_EVENTS);
//        const uint8_t tx_buffer[] = {BYTE_A, BYTE_B};
//        slave->set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
//        uint8_t rx_buffer[] = {0x00, 0x00};
//
//        // WHEN an I2C transaction happens
//        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [&rx_buffer](){
//            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);
//        });
//
//        // THEN the master sets the tLOW and tHIGH correctly
//        // AND the clock speed is no faster than the stated frequency
////        print_traces(trace, expected_trace);
//        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));
//        // Assert in range
//        auto analysis = analysis::I2CTimingAnalyser::analyse(trace);
//        TEST_ASSERT_TRUE(analysis.scl_low_time.meets_specification(parameters.times.scl_low_time));
//        // TODO: Can we standardise the design check?
////        TEST_ASSERT_TRUE(analysis.scl_low_time.matches_design(design_parameters.scl_low_time));
//        TEST_ASSERT_UINT32_WITHIN(master_design_parameters.scl_low_time*0.1, master_design_parameters.scl_low_time, analysis.scl_low_time.average());
//
//        TEST_ASSERT_EQUAL_UINT32_MESSAGE(SIZE_MAX, 1, "Not implemented");
//    }

    static void test_suite(I2CMaster& master_, I2CSlave& slave_) {
        master = &master_;
        slave = &slave_;
//        RUN_TEST(start_bit);
//        RUN_TEST(master_clock_speed);
    }

    static void log_value(const char* msg, uint32_t expected, uint32_t actual) {
        Serial.print(msg);Serial.print(". Expected ");Serial.print(expected);Serial.print(". Actual ");Serial.println(actual);
    }

    void test() final {
        // Run all combinations of supported frequencies
        // and supported I2C ports
        Serial.println("Testing at 100 kHz");
        frequency = 100'000;
        parameters = common::i2c_specification::StandardMode;
        master_design_parameters = StandardModeDesignParameters;
        Serial.println("Testing Master & Slave1");
        test_suite(Master, Slave1);
//        Serial.println("Testing Master1 & Slave");
//        test_suite(Master1, Slave);
//        Serial.println(".");
//
//        Serial.println("Testing at 400 kHz");
//        frequency = 400'000;
//        parameters = common::i2c_specification::FastMode;
//        master_design_parameters = FastModeDesignParameters;
//        Serial.println("Testing Master & Slave1");
//        test_suite(Master, Slave1);
//        Serial.println("Testing Master1 & Slave");
//        test_suite(Master1, Slave);
//        Serial.println(".");

        // TODO: Tests currently fail at 1 MHz
        // Seems to be sensitive to the bus timings and how long BusRecorder
        // takes to capture an event. Disable for now.
//        Serial.println("Testing at 1 MHz");
//        frequency = 1'000'000;
//        parameters = common::i2c_specification::FastModePlus;
//        master_design_parameters = FastModePlusDesignParameters;
//        Serial.println("Testing Master & Slave1");
//        test_suite(Master, Slave1);
//        Serial.println("Testing Master1 & Slave");
//        test_suite(Master1, Slave);
    }

    MasterSignalsTest() : E2ETestBase(__FILE__) {};
};

// Define statics
I2CMaster* e2e::loopback::signals::MasterSignalsTest::master;
I2CSlave* e2e::loopback::signals::MasterSignalsTest::slave;
uint32_t e2e::loopback::signals::MasterSignalsTest::frequency;
common::i2c_specification::I2CParameters e2e::loopback::signals::MasterSignalsTest::parameters;
analysis::MasterDesignParameters e2e::loopback::signals::MasterSignalsTest::master_design_parameters = StandardModeDesignParameters;

// Define statics
//int SlaveSignalTest::value;

} // signals
} // loopback
} // e2e
#endif //TEENSY4_I2C_TEST_E2E_MASTER_SIGNALS_H
