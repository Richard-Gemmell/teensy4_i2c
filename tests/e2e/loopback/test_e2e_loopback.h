#define TEENSY_I2C_E2E_TEST_LOOPBACK
#ifdef TEENSY_I2C_E2E_TEST_LOOPBACK

#include <Arduino.h>
#include <unity.h>
#include <cstdint>
#include <i2c_driver.h>
#include "imx_rt1060/imx_rt1060_i2c_driver.h"
#include "utils/test_suite.h"

namespace e2e {
namespace loopback {

class LoopbackTest : public TestSuite {
public:
    // Setup a master on port 0 to talk to a slave on port 1
    static I2CMaster* master;
    static uint8_t rx_buffer[1];

    const static uint8_t slave_address = 0x53;
    static I2CSlave* slave;
    static uint8_t tx_buffer[1];

    static void finish() {
        elapsedMillis timeout;
        while (timeout < 200) {
            if (master->finished()) {
                return;
            }
        }
        Serial.println("Master: ERROR timed out waiting for transfer to finish.");
    }

    void setUp() override {
        master = &Master;
        slave = &Slave1;
    }

    static void test_write() {
        // GIVEN a master and slave
        master->begin(100 * 1000U);
        slave->set_transmit_buffer(tx_buffer, sizeof(tx_buffer));
        slave->listen(slave_address);

        // WHEN the master reads a single byte from the slave
        rx_buffer[0] = 0x00;
        master->read_async(slave_address, rx_buffer, sizeof(rx_buffer), true);
        finish();
        slave->stop_listening();
        master->end();

        // THEN the data is received correctly
        TEST_ASSERT_EQUAL(0x58, rx_buffer[0]);
    }

    void test() final {
        RUN_TEST(test_write);
    }

    LoopbackTest() : TestSuite(__FILE__) {};
};

// Define statics
I2CMaster* e2e::loopback::LoopbackTest::master;
uint8_t e2e::loopback::LoopbackTest::rx_buffer[1] = {0x00};

I2CSlave* e2e::loopback::LoopbackTest::slave;
uint8_t e2e::loopback::LoopbackTest::tx_buffer[1] = {0x58};

}
}
#endif //TEENSY_I2C_E2E_TEST_LOOPBACK