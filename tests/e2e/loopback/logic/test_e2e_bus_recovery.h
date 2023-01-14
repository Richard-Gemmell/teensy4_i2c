// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_BUS_RECOVERY_H
#define TEENSY4_I2C_TEST_E2E_BUS_RECOVERY_H

#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"
#include "e2e/loopback/loopback_test_base.h"
#include <imx_rt1060/imx_rt1060_i2c_driver.h>
#include <bus_trace/bus_trace_builder.h>

namespace e2e {
namespace loopback {
namespace logic {

// Tests the most common scenarios.
class BusRecoveryTest : public LoopbackTestBase {
public:
    const static uint8_t BYTE_A = 0x58; // 0101 1000
    const static uint8_t BYTE_B = 0xA7; // 1010 0111
    const static uint8_t ADDRESS = 0x53;// 0101 0011
    static uint32_t frequency;
    static I2CMaster* master;
    static I2CSlave* slave;

    void setUp() override {
        LoopbackTestBase::setUp();
        Loopback::enable_pullup(Loopback::PIN_SCL_120_ns);
        Loopback::enable_pullup(Loopback::PIN_SDA_120_ns);
        master = &Master;
        slave = &Slave1;
    }

    static void bus_becomes_idle_if_another_master_stops_responding() {
        // This test demonstrates that the master decides that the bus
        // is idle when both lines are high irrespective of the value
        // of BUSIDLE. Section `47.3.1.2 Master operations` implies that
        // BUSIDLE must be > 0 for this to work.
        // Confirmed Jan 2023 that this test fails if BUSIDLE == 0
        common::hal::TeensyPin sda(PIN_SNIFF_SDA, OUTPUT_OPENDRAIN);
        sda.set();
        common::hal::TeensyPin scl(PIN_SNIFF_SCL, OUTPUT_OPENDRAIN);
        scl.set();
        delayNanoseconds(1000);
        bus_trace::BusTrace trace(MAX_EVENTS);
        const uint8_t tx_buffer[] = {BYTE_A, BYTE_B};
        slave->set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
        uint8_t rx_buffer[] = {0x00, 0x00};

        // WHEN the bus idle time has expired
        trace_i2c_transaction(master, frequency, slave, ADDRESS, trace, [&rx_buffer, &sda, &scl](){
            // GIVEN another master stopped transmitting without sending a STOP
            // Simulate a START
            delayNanoseconds(1000);
            sda.clear();
            delayNanoseconds(1000);
            scl.clear();

            // Master tries to start a transaction while the other master is busy
            master->read_async(ADDRESS, rx_buffer, sizeof(rx_buffer), true);

            // Other master starts a data bit and then just gives up leaving both lines HIGH
            delayNanoseconds(1000);
            sda.set();
            delayNanoseconds(1000);
            scl.set();

            // Master is free to continue
        });

        // THEN this master was able to start the transaction
        TEST_ASSERT(master->finished());
        TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_buffer, rx_buffer, sizeof(tx_buffer));
    }

    void test() final {
        Serial.println("100 kHz");
        frequency = 100'000;
        RUN_TEST(bus_becomes_idle_if_another_master_stops_responding);

        Serial.println("400 kHz");
        frequency = 400'000;
        RUN_TEST(bus_becomes_idle_if_another_master_stops_responding);

        Serial.println("1 MHz");
        frequency = 1'000'000;
        RUN_TEST(bus_becomes_idle_if_another_master_stops_responding);
    }

    BusRecoveryTest() : LoopbackTestBase(__FILE__) {};
};

// Define statics
uint32_t e2e::loopback::logic::BusRecoveryTest::frequency;
I2CMaster* e2e::loopback::logic::BusRecoveryTest::master;
I2CSlave* e2e::loopback::logic::BusRecoveryTest::slave;

} // signals
} // loopback
} // e2e

#endif //TEENSY4_I2C_TEST_E2E_BUS_RECOVERY_H
