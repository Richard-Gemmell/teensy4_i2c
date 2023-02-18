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
    static const uint8_t config_register = 0x00;
    static const uint8_t manufacturer_id_register = 0xFE;
    static const uint8_t die_id_register = 0xFF;
    static uint32_t frequency;
    static I2CMaster& master;
    static I2CDevice ina260;

public:
    void setUp() final {
        master.begin(frequency);
    }

    void tearDown() final {
        master.end();
    }

    static void test_can_read_manufacturer_id() {
        // Read the Manufacturer ID
        uint16_t value;
        ina260.read(manufacturer_id_register, &value, true);

        TEST_ASSERT_EQUAL_HEX16(0x5449, value);
    }

    static void test_can_read_die_id() {
        // Read the Die ID
        uint16_t value;
        ina260.read(die_id_register, &value, true);

        TEST_ASSERT_EQUAL_HEX16(0x2270, value);
    }

    static void test_can_write_config() {
        // WHEN we change the configuration
        uint16_t config;
        const uint16_t new_config = 0x6126;
        ina260.write(config_register, new_config, true);

        // THEN the device returns the new configuration
        ina260.read(config_register, &config, true);
        TEST_ASSERT_EQUAL_HEX16(new_config, config);

        // WHEN we change the configuration back to the default
        const uint16_t default_config = 0x6127;
        ina260.write(config_register, default_config, true);

        // THEN the device returns the original configuration
        ina260.read(config_register, &config, true);
        TEST_ASSERT_EQUAL_HEX16(default_config, config);
    }

    void test() final {
        Serial.println("100 kHz");
        frequency = 100 * 1000U;
        RUN_TEST(test_can_read_manufacturer_id);
        RUN_TEST(test_can_read_die_id);
        RUN_TEST(test_can_write_config);

        Serial.println("400 kHz");
        frequency = 400 * 1000U;
        RUN_TEST(test_can_read_manufacturer_id);
        RUN_TEST(test_can_read_die_id);
        RUN_TEST(test_can_write_config);

        Serial.println("1 MHz");
        frequency = 1000 * 1000U;
        RUN_TEST(test_can_read_manufacturer_id);
        RUN_TEST(test_can_read_die_id);
        RUN_TEST(test_can_write_config);
    }

    Ina260EndToEndTest() : TestSuite(__FILE__) {};
};

// Define statics
I2CMaster& Ina260EndToEndTest::master = Master;
I2CDevice Ina260EndToEndTest::ina260(Ina260EndToEndTest::master, Ina260EndToEndTest::slave_address, _BIG_ENDIAN);
uint32_t Ina260EndToEndTest::frequency;
}
}
#endif  // TEENSY4_I2C_TEST_E2E_OTHER_DEVICES_INA_260_H
