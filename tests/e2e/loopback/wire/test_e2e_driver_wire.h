// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_TEST_E2E_LOOPBACK_WIRE_E2E_DRIVER_WIRE_H
#define TEENSY4_I2C_TEST_E2E_LOOPBACK_WIRE_E2E_DRIVER_WIRE_H

#include <unity.h>
#include <Arduino.h>
#include <i2c_device.h>
#include "utils/test_suite.h"

namespace e2e {
namespace loopback {
namespace wire {

// Miscellaneous tests of I2CDriverWire
// Note that some behaviour is tested elsewhere. e.g. pullup configuration
class DriverWireTest : public TestSuite {
public:
    static volatile uint8_t callback_count;
    static const uint8_t slave_address = 0x20;

    static bool finish(I2CMaster& master) {
        elapsedMillis timeout;
        while (timeout <= 200) {
            if (master.finished()) {
                return true;
            }
        }
        return false;
    }

    static uint8_t read_byte(I2CMaster& master, uint8_t address) {
        uint8_t result = 0;
        master.read_async(address, &result, sizeof(result), true);
        if (!finish(master)) {
            Serial.println("Timed out waiting for read to complete.");
        }
        return result;
    }

    static void write_byte(I2CMaster& master, uint8_t address) {
        uint8_t value = 'S';
        master.write_async(address, &value, sizeof(value), true);
        if (!finish(master)) {
            Serial.println("Timed out waiting for write to complete.");
        }
    }

    static void test_end_clears_before_transmit_callback_for_slave() {
        // GIVEN we have a slave listening to a master
        I2CDriverWire& wire_slave = Wire1;
        I2CSlave& slave = Slave1;
        I2CMaster& master = Master;
        master.begin(100'000);
        callback_count = 0;
        wire_slave.begin(slave_address);
        wire_slave.onRequest([](){
            Wire1.write('A');
            callback_count++;
        });
        uint8_t byte1 = read_byte(master, slave_address);

        // WHEN we call end()
        wire_slave.end();

        // THEN end() removes the 'before_transmit' callback
        slave.listen(slave_address);
        uint8_t byte2 = read_byte(master, slave_address);
        if(byte2 == 0) {
            Serial.println("Didn't get a result from the second call.");
        }
        slave.stop_listening();
        master.end();
        TEST_ASSERT_NOT_EQUAL_MESSAGE(0, byte1, "First read failed");
        TEST_ASSERT_NOT_EQUAL_MESSAGE(0, byte2, "Second read failed");
        TEST_ASSERT_EQUAL(1, callback_count);
    }

    static void test_end_clears_after_receive_callback_for_slave() {
        // GIVEN we have a slave listening to a master
        I2CDriverWire& wire_slave = Wire1;
        I2CSlave& slave = Slave1;
        I2CMaster& master = Master;
        master.begin(100'000);
        callback_count = 0;
        wire_slave.begin(slave_address);
        wire_slave.onReceive([](int len){
            callback_count++;
        });
        write_byte(master, slave_address);

        // WHEN we call end()
        wire_slave.end();

        // THEN end() removes the 'after_receive' callback
        slave.listen(slave_address);
        write_byte(master, slave_address);
        master.end();
        TEST_ASSERT_EQUAL(1, callback_count);
    }

    // Include all the tests here
    void test() final {
        RUN_TEST(test_end_clears_before_transmit_callback_for_slave);
        RUN_TEST(test_end_clears_after_receive_callback_for_slave);
    }

    DriverWireTest() : TestSuite(__FILE__) {};
};

volatile uint8_t e2e::loopback::wire::DriverWireTest::callback_count = 0;

}
}
}

#endif //TEENSY4_I2C_TEST_E2E_LOOPBACK_WIRE_E2E_DRIVER_WIRE_H
