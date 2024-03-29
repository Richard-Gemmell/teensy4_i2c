// Copyright © 2021-2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#define TEENSY_I2C_UNIT_TEST_I2C_DEVICE_TEST
#ifdef TEENSY_I2C_UNIT_TEST_I2C_DEVICE_TEST

#include <Arduino.h>
#include <unity.h>
#include <cstdint>
#include "i2c_device.h"
#include "utils/test_suite.h"

struct TestBuffer {
    bool _read = false;
    uint16_t _address = 0;
    uint8_t _buffer[4] = {};
    size_t _num_bytes = 0;
    bool _send_stop = false;

    void set(bool read, uint16_t address, const uint8_t* buffer, size_t num_bytes, bool send_stop) {
        _read = read;
        _address = address;
        _num_bytes = num_bytes;
        memcpy(&_buffer, buffer, _num_bytes);
        _send_stop = send_stop;
    }

    void assert_unused() {
        assert_details(false, 0, false);
        uint8_t blank[] = {0, 0, 0, 0};
        TEST_ASSERT_EQUAL_MEMORY(blank, _buffer, sizeof(_buffer));
    }

    void assert_equals(bool read, uint16_t address, uint8_t buffer[], size_t num_bytes, bool send_stop) {
        assert_details(read, address, send_stop);
        TEST_ASSERT_EQUAL_MEMORY(buffer, _buffer, num_bytes);
    }

    void assert_details(bool read, uint16_t address, bool send_stop) const {
        TEST_ASSERT_EQUAL(read, _read);
        TEST_ASSERT_EQUAL(address, _address);
        TEST_ASSERT_EQUAL(send_stop, _send_stop);
    }

    void reset() {
        _read = false;
        _address = 0;
        memset(_buffer, 0, sizeof(_buffer));
        _num_bytes = 0;
        _send_stop = false;
    }
};

class DummyI2CMaster : public I2CMaster {
public:
    virtual ~DummyI2CMaster() = default;

    void set_error(I2CError error) {
        _error = error;
    }

    void begin(uint32_t frequency) override {
    };

    void end() override {
    };

    bool finished() override {
        return _finished;
    };

    size_t get_bytes_transferred() override {
        return -1;
    }

    void write_async(uint8_t address, const uint8_t* buffer, size_t num_bytes, bool send_stop) override {
        copy_to_next_buffer(false, address, buffer, num_bytes, send_stop);
    };

    void read_async(uint8_t address, uint8_t* buffer, size_t num_bytes, bool send_stop) override {
        memcpy(buffer, read_data, num_bytes);
        copy_to_next_buffer(true, address, buffer, num_bytes, send_stop);
    };

    void copy_to_next_buffer(bool read, uint8_t address, const uint8_t* buffer, size_t num_bytes, bool send_stop){
        if(next_buffer < size_t(buffers)) {
            buffers[next_buffer++].set(read, address, buffer, num_bytes, send_stop);
        }
    }

    void reset() {
        _error = I2CError::ok;
        next_buffer = 0;
        _finished = true;
        buffers[0].reset();
        buffers[1].reset();
    }

    TestBuffer buffers[2];
    size_t next_buffer = 0;
    uint8_t read_data[8] = {0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};

    bool _finished = true;
};

class I2CDeviceTest : public TestSuite {
public:
    static DummyI2CMaster* dummy;
    static uint16_t address;
    static I2CDevice* device;
    static I2CDevice* big_endian_device;

    void setUp() override {
        dummy = new DummyI2CMaster();
        device = new I2CDevice(*dummy, address);
        big_endian_device = new I2CDevice(*dummy, address, BIG_ENDIAN);
    }

    void tearDown() override {
        delete(dummy);
        dummy = nullptr;
        delete(device);
        device = nullptr;
        delete(big_endian_device);
        big_endian_device = nullptr;
    }

    static void test_write() {
        uint8_t reg = 0x12;
        int16_t value = 0x01AB;

        bool success = device->write(reg, (uint8_t*)&value, sizeof(value), true);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[3] = {reg, 0xAB, 0x01};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), true);
    }

    static void test_read_fails_to_send_register() {
        uint8_t reg = 0xDD;
        uint8_t rx_buffer[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        dummy->set_error(I2CError::arbitration_lost);

        bool success = device->read(reg, rx_buffer, sizeof(rx_buffer), true);

        TEST_ASSERT_FALSE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_unused();
        // Wipes target buffer if read fails.
        uint8_t expected_rx_buffer[8] = {};
        TEST_ASSERT_EQUAL_MEMORY(expected_rx_buffer, rx_buffer, sizeof(rx_buffer));
    }

    static void test_read() {
        uint8_t reg = 0xDD;
        uint8_t rx_buffer[8] = {};

        bool success = device->read(reg, rx_buffer, sizeof(rx_buffer), true);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_equals(true, address, rx_buffer, sizeof(rx_buffer), true);
        uint8_t expected_rx_buffer[8] = {0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};
        TEST_ASSERT_EQUAL_MEMORY(expected_rx_buffer, rx_buffer, sizeof(rx_buffer));
    }

    static void test_write_uint8t() {
        uint8_t reg = 0x12;
        uint8_t value = 0xDE;

        bool success = device->write(reg, value, false);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[2] = {reg, value};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), false);
    }

    static void test_write_int8t() {
        uint8_t reg = 0x10;
        int8_t value = 0xDE;

        bool success = device->write(reg, value, true);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[2] = {reg, (uint8_t)value};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), true);
    }

    static void test_write_uint16t() {
        uint8_t reg = 0x12;
        uint16_t value = 0xABDE;

        bool success = device->write(reg, value, true);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[3] = {reg, 0xDE, 0xAB};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), true);
    }

    static void test_write_uint16t_big_endian() {
        uint8_t reg = 0x12;
        uint16_t value = 0xABDE;

        bool success = big_endian_device->write(reg, value, false);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[3] = {reg, 0xAB, 0xDE};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), false);
    }

    static void test_write_int16t() {
        uint8_t reg = 0x12;
        int16_t value = 0xABDE;

        bool success = device->write(reg, value, true);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[3] = {reg, 0xDE, 0xAB};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), true);
    }

    static void test_write_int16t_big_endian() {
        uint8_t reg = 0x12;
        int16_t value = 0xABDE;

        bool success = big_endian_device->write(reg, value, false);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[3] = {reg, 0xAB, 0xDE};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), false);
    }

    static void test_write_uint32t() {
        uint8_t reg = 0x12;
        uint32_t value = 0xABDE1234;

        bool success = device->write(reg, value, true);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[5] = {reg, 0x34, 0x12, 0xDE, 0xAB};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), true);
    }

    static void test_write_uint32t_big_endian() {
        uint8_t reg = 0x12;
        uint32_t value = 0xABDE1234;

        bool success = big_endian_device->write(reg, value, false);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[5] = {reg, 0xAB, 0xDE, 0x12, 0x34};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), false);
    }

    static void test_write_int32t() {
        uint8_t reg = 0x12;
        int32_t value = 0xABDE1234;

        bool success = device->write(reg, value, true);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[5] = {reg, 0x34, 0x12, 0xDE, 0xAB};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), true);
    }

    static void test_write_int32t_big_endian() {
        uint8_t reg = 0x12;
        int32_t value = 0xABDE1234;

        bool success = big_endian_device->write(reg, value, false);

        TEST_ASSERT_TRUE(success);
        uint8_t expected_buffer[5] = {reg, 0xAB, 0xDE, 0x12, 0x34};
        dummy->buffers[0].assert_equals(false, address, expected_buffer, sizeof(expected_buffer), false);
    }

    static void test_read_uin8t() {
        uint8_t reg = 0x12;
        uint8_t value = 0;

        bool success = device->read(reg, &value, true);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_equals(true, address, &value, sizeof(value), true);
        TEST_ASSERT_EQUAL(0x08, value);
    }

    static void test_read_int8t() {
        uint8_t reg = 0x12;
        int8_t value = 0;

        bool success = device->read(reg, &value, true);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_equals(true, address, (uint8_t*)&value, sizeof(value), true);
        TEST_ASSERT_EQUAL((int8_t)0x08, value);
    }

    static void test_read_uin16t() {
        uint8_t reg = 0x12;
        uint16_t value = 0;

        bool success = device->read(reg, &value, true);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_equals(true, address, (uint8_t*)&value, sizeof(value), true);
        TEST_ASSERT_EQUAL(0x0908, value);
    }

    static void test_read_uin16t_big_endian() {
        uint8_t reg = 0x12;
        uint16_t value = 0;

        bool success = big_endian_device->read(reg, &value, true);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_details(true, address, true);
        TEST_ASSERT_EQUAL(0x0809, value);
    }

    static void test_read_in16t() {
        uint8_t reg = 0x12;
        int16_t value = 0;

        bool success = device->read(reg, &value, true);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_equals(true, address, (uint8_t*)&value, sizeof(value), true);
        TEST_ASSERT_EQUAL(0x0908, value);
    }

    static void test_read_in16t_big_endian() {
        uint8_t reg = 0x16;
        int16_t value = 0;

        bool success = big_endian_device->read(reg, &value, false);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_details(true, address, false);
        TEST_ASSERT_EQUAL(0x0809, value);
    }

    static void test_read_uin32t() {
        uint8_t reg = 0x12;
        uint32_t value = 0;

        bool success = device->read(reg, &value, true);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_equals(true, address, (uint8_t*)&value, sizeof(value), true);
        TEST_ASSERT_EQUAL(0x0B0A0908, value);
    }

    static void test_read_uin32t_big_endian() {
        uint8_t reg = 0x12;
        uint32_t value = 0;

        bool success = big_endian_device->read(reg, &value, true);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_details(true, address, true);
        TEST_ASSERT_EQUAL(0x08090A0B, value);
    }

    static void test_read_in32t() {
        uint8_t reg = 0x12;
        int32_t value = 0;

        bool success = device->read(reg, &value, true);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_equals(true, address, (uint8_t*)&value, sizeof(value), true);
        TEST_ASSERT_EQUAL(0x0B0A0908, value);
    }

    static void test_read_in32t_big_endian() {
        uint8_t reg = 0x12;
        int32_t value = 0;

        bool success = big_endian_device->read(reg, &value, true);

        TEST_ASSERT_TRUE(success);
        dummy->buffers[0].assert_equals(false, address, &reg, sizeof(reg), false);
        dummy->buffers[1].assert_details(true, address, true);
        TEST_ASSERT_EQUAL(0x08090A0B, value);
    }

    void test() final {
        RUN_TEST(test_write);
        RUN_TEST(test_read_fails_to_send_register);
        RUN_TEST(test_read);

        RUN_TEST(test_write_uint8t);
        RUN_TEST(test_write_int8t);
        RUN_TEST(test_write_uint16t);
        RUN_TEST(test_write_uint16t_big_endian);
        RUN_TEST(test_write_int16t);
        RUN_TEST(test_write_int16t_big_endian);
        RUN_TEST(test_write_uint32t);
        RUN_TEST(test_write_uint32t_big_endian);
        RUN_TEST(test_write_int32t);
        RUN_TEST(test_write_int32t_big_endian);

        RUN_TEST(test_read_uin8t);
        RUN_TEST(test_read_int8t);
        RUN_TEST(test_read_uin16t);
        RUN_TEST(test_read_uin16t_big_endian);
        RUN_TEST(test_read_in16t);
        RUN_TEST(test_read_in16t_big_endian);
        RUN_TEST(test_read_uin32t);
        RUN_TEST(test_read_uin32t_big_endian);
        RUN_TEST(test_read_in32t);
        RUN_TEST(test_read_in32t_big_endian);
    }

    I2CDeviceTest() : TestSuite(__FILE__) {};
};

// Define statics
DummyI2CMaster* I2CDeviceTest::dummy;
uint16_t I2CDeviceTest::address = 0x22;
I2CDevice* I2CDeviceTest::device;
I2CDevice* I2CDeviceTest::big_endian_device;

#endif //TEENSY_I2C_UNIT_TEST_I2C_DEVICE_TEST