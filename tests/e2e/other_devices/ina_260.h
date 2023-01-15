// Copyright ©2023 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_OTHER_DEVICES_INA_260_H
#define TEENSY4_I2C_TEST_E2E_OTHER_DEVICES_INA_260_H

#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"
#include <i2c_driver.h>
#include <imx_rt1060/imx_rt1060_i2c_driver.h>

namespace e2e {
namespace other_devices {

class Ina260EndToEndTest : public TestSuite {
// The slave is an INA 260 current sensor
    static const uint16_t slave_address = 0x40;
    static const uint8_t manufacturer_id_register = 0xFE;
    static const uint8_t die_id_register = 0xFF;
    static uint32_t frequency;
    static I2CMaster& master;

public:
    void setUp() final {
        master.begin(frequency);
    }

    void tearDown() final {
        master.end();
    }

    static bool finished_ok() {
        elapsedMillis timeout;
        while (timeout < 3) {
            if (master.finished()){
                break;
            }
        }
        return master.finished() && !master.has_error();
    }

    static uint16_t get_int_from_buffer(uint8_t rx_buffer[2]) {
        uint16_t result = ((uint16_t)rx_buffer[0] << 8U) + ((uint16_t)rx_buffer[1]);
        return result;
    }

    static uint16_t read_register(uint8_t reg_num) {
        uint8_t rx_buffer[2] = {0, 0};

        // Send register number to read
        master.write_async(slave_address, (uint8_t*)&reg_num, sizeof(uint8_t), false);
        if (finished_ok()) {
            // Read the data
            master.read_async(slave_address, rx_buffer, sizeof(rx_buffer), true);
            if (finished_ok()) {
                return get_int_from_buffer(rx_buffer);
            }
        }
        return 0;
    }

    static void test_can_read_manufacturer_id() {
        // Read the Manufacturer ID
        uint16_t value = read_register(manufacturer_id_register);

        TEST_ASSERT_EQUAL(0x5449, value);
    }

    static void test_can_read_die_id() {
        // Read the Die ID
        uint16_t value = read_register(die_id_register);

        TEST_ASSERT_EQUAL(0x2270, value);
    }

    void test() final {
        Serial.println("100 kHz");
        frequency = 100 * 1000U;
        RUN_TEST(test_can_read_manufacturer_id);
        RUN_TEST(test_can_read_die_id);

        Serial.println("400 kHz");
        frequency = 400 * 1000U;
        RUN_TEST(test_can_read_manufacturer_id);
        RUN_TEST(test_can_read_die_id);

        Serial.println("1 MHz");
        frequency = 1000 * 1000U;
        RUN_TEST(test_can_read_manufacturer_id);
        RUN_TEST(test_can_read_die_id);
    }

    Ina260EndToEndTest() : TestSuite(__FILE__) {};
};

// Define statics
I2CMaster& Ina260EndToEndTest::master = Master;
uint32_t Ina260EndToEndTest::frequency;
}
}
#endif  // TEENSY4_I2C_TEST_E2E_OTHER_DEVICES_INA_260_H
