//#define TEST_I2C_REGISTER_SLAVE
#ifdef TEST_I2C_REGISTER_SLAVE

#include <Arduino.h>
#include <unity.h>
#include <cstdint>
#include "i2c_register_slave.h"

// HACK to solve a linker problem.
// I guess exceptions are enabled in the Test build
unsigned __exidx_start;
unsigned __exidx_end;

class DummyI2CSlave : public I2CSlave {
public:
    void listen(uint8_t slave_address) override {
        address = slave_address;
    };

    void listen(uint8_t first_address, uint8_t second_address) {
        // Not implemented as not required by I2CRegisterSlave
    }

    void listen_range(uint8_t first_address, uint8_t last_address) {
        // Not implemented as not required by I2CRegisterSlave
    }

    void stop_listening() override {};

    inline void after_receive(std::function<void(size_t length, uint16_t address)> callback) override {
        after_receive_callback = callback;
    }

    void before_transmit(std::function<void(uint16_t address)> callback) override {
    };

    void after_transmit(std::function<void(uint16_t address)> callback) override {
        after_transmit_callback = callback;
    };

    void set_transmit_buffer(uint8_t* buffer, size_t size) override {
        latest_tx_buffer = buffer;
        latest_tx_buffer_size = size;
    };

    void set_receive_buffer(uint8_t* buffer, size_t size) override {
        latest_rx_buffer = buffer;
        latest_rx_buffer_size = size;
    }

    void write(uint8_t reg_num, uint8_t* buffer, size_t len) {
        if (latest_rx_buffer) {
            latest_rx_buffer[0] = reg_num;
            memcpy(latest_rx_buffer+1, buffer, min(len, latest_rx_buffer_size-1));
            if(after_receive_callback) {
                after_receive_callback(len+1, address);
            }
        }
    }

    void write_reg_number(uint8_t reg_num) {
        if (latest_rx_buffer != nullptr && latest_rx_buffer_size > 0) {
            latest_rx_buffer[0] = reg_num;
            if(after_receive_callback) {
                after_receive_callback(sizeof(uint8_t), address);
            }
        }
    }

    void write_value(uint8_t* buffer, size_t len) {
        if (latest_rx_buffer) {
            memcpy(latest_rx_buffer, buffer, min(len, latest_rx_buffer_size));
            if(after_receive_callback) {
                after_receive_callback(len, address);
            }
        }
    }

    void read_value(uint8_t* buffer, size_t len) {
        if (latest_tx_buffer) {
            memcpy(buffer, latest_tx_buffer, min(len, latest_tx_buffer_size));
            if(after_transmit_callback) {
                after_transmit_callback(address);
            }
        }
    }

    void reset() {
        latest_rx_buffer = nullptr;
        latest_rx_buffer_size = 0;
        latest_tx_buffer = nullptr;
        latest_tx_buffer_size = 0;
    }

    uint16_t address = 0;
    uint8_t* latest_rx_buffer = nullptr;
    size_t latest_rx_buffer_size = 0;
    uint8_t* latest_tx_buffer = nullptr;
    size_t latest_tx_buffer_size = 0;

private:
    std::function<void(size_t length, uint16_t address)> after_receive_callback = nullptr;
//    std::function<void(uint16_t address)> before_transmit_callback = nullptr;
    std::function<void(uint16_t address)> after_transmit_callback = nullptr;
};

DummyI2CSlave dummy;
uint16_t address = 0xDD;
uint8_t settings[4] = {0x00, 0x00, 0x00, 0x00};
uint8_t filled_settings[4] = {0xAA, 0xBB, 0xCC, 0xDD};
uint8_t blank_read_only[4] = {0x00, 0x00, 0x00, 0x00};
uint8_t read_only[4] = {0x0A, 0x0B, 0x0C, 0x0D};

void setUp() {
    dummy.reset();
    memset(settings, 0, sizeof(settings));
}

void test_listen_calls_listen_on_driver() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    TEST_ASSERT_EQUAL(address, dummy.address);
}

void test_master_can_write_to_mutable_register() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint16_t value = 0xBBAA;
    dummy.write_reg_number(0x01);
    dummy.write_value((uint8_t*)&value, sizeof(value));

    uint8_t expected[sizeof(settings)] = {0x00, 0xAA, 0xBB, 0x00};
    TEST_ASSERT_EQUAL_MEMORY(expected, settings, sizeof(settings));
}

void test_master_can_write_to_multiple_registers() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t value_1 = 0xAA;
    dummy.write_reg_number(0x01);
    dummy.write_value((uint8_t*)&value_1, sizeof(value_1));

    uint8_t value_2 = 0xFF;
    dummy.write_reg_number(0x03);
    dummy.write_value((uint8_t*)&value_2, sizeof(value_2));

    uint8_t expected[sizeof(settings)] = {0x00, 0xAA, 0x00, 0xFF};
    TEST_ASSERT_EQUAL_MEMORY(expected, settings, sizeof(settings));
}

void test_master_cannot_write_to_non_existent_register() {
    uint8_t big_settings[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t claimed_settings_size = 2;
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, big_settings, claimed_settings_size, read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t reg = 2;    // beyond the end of the claimed size of big_settings
    uint32_t value = 0xAABBCCDD;
    dummy.write_reg_number(reg);
    dummy.write_value((uint8_t*)&value, sizeof(value));

    uint8_t expected[sizeof(big_settings)] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_MEMORY(expected, big_settings, sizeof(big_settings));
}

void test_master_cannot_write_too_many_bytes_to_a_register() {
    uint8_t big_settings[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t claimed_settings_size = 2;
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, big_settings, claimed_settings_size, read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t reg = 0x01;    // beyond the end of the claimed size of big_settings
    uint32_t value = 0xAABBCCDD;
    dummy.write_reg_number(reg);
    dummy.write_value((uint8_t*)&value, sizeof(value));

    uint8_t expected[sizeof(big_settings)] = {0x00, 0xDD, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_MEMORY(expected, big_settings, sizeof(big_settings));
}

void test_master_ignores_repeated_write_to_mutable_register() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t value_1 = 0xAA;
    dummy.write_reg_number(0x01);
    dummy.write_value((uint8_t*)&value_1, sizeof(value_1));

    // Should ignore this write because we haven't specified the register.
    uint8_t value_2 = 0xFF;
    dummy.write_value((uint8_t*)&value_2, sizeof(value_2));

    uint8_t expected[sizeof(settings)] = {0x00, 0xAA, 0x00, 0x00};
    TEST_ASSERT_EQUAL_MEMORY(expected, settings, sizeof(settings));
}

void test_master_can_write_register_in_single_transaction() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint16_t value = 0xBBAA;
    dummy.write(0x01, (uint8_t*)&value, sizeof(value));

    uint8_t expected[sizeof(settings)] = {0x00, 0xAA, 0xBB, 0x00};
    TEST_ASSERT_EQUAL_MEMORY(expected, settings, sizeof(settings));
}

void test_master_can_write_to_multiple_registers_with_single_transactions() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t value_1 = 0xAA;
    dummy.write(0x01, (uint8_t*)&value_1, sizeof(value_1));

    uint8_t value_2 = 0xFF;
    dummy.write(0x03, (uint8_t*)&value_2, sizeof(value_2));

    uint8_t expected[sizeof(settings)] = {0x00, 0xAA, 0x00, 0xFF};
    TEST_ASSERT_EQUAL_MEMORY(expected, settings, sizeof(settings));
}

void test_ignores_single_transaction_write_to_non_existent_register() {
    uint8_t big_settings[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t claimed_settings_size = 2;
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, big_settings, claimed_settings_size, read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t reg = 2;    // beyond the end of the claimed size of big_settings
    uint32_t value = 0xAABBCCDD;
    dummy.write(reg, (uint8_t*)&value, sizeof(value));

    uint8_t expected[sizeof(big_settings)] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_MEMORY(expected, big_settings, sizeof(big_settings));
}

void test_master_cannot_write_too_many_bytes_to_a_register_with_single_transaction() {
    uint8_t big_settings[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t claimed_settings_size = 2;
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, big_settings, claimed_settings_size, read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t reg = 0x01;    // beyond the end of the claimed size of big_settings
    uint32_t value = 0xAABBCCDD;
    dummy.write(reg, (uint8_t*)&value, sizeof(value));

    uint8_t expected[sizeof(big_settings)] = {0x00, 0xDD, 0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_MEMORY(expected, big_settings, sizeof(big_settings));
}

void test_master_can_read_from_mutable_register() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, filled_settings, sizeof(filled_settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint16_t value = 0;
    dummy.write_reg_number(0x01);
    dummy.read_value((uint8_t*)&value, sizeof(value));

    TEST_ASSERT_EQUAL(0xCCBB, value);
}

void test_master_can_read_from_multiple_mutable_registers() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, filled_settings, sizeof(filled_settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint16_t value_1 = 0;
    dummy.write_reg_number(0x01);
    dummy.read_value((uint8_t*)&value_1, sizeof(value_1));
    TEST_ASSERT_EQUAL(0xCCBB, value_1);

    uint8_t value_2 = 0;
    dummy.write_reg_number(0x00);
    dummy.read_value((uint8_t*)&value_2, sizeof(value_2));
    TEST_ASSERT_EQUAL(0xAA, value_2);
}

void test_master_cannot_read_from_non_existent_register() {
    uint8_t long_read_only[] = {0x0A, 0x0B, 0x0C, 0x0D, 0x11, 0x11, 0x11, 0x11};
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), long_read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t reg = sizeof(settings) + sizeof(read_only);    // beyond the end of the claimed size of buffers
    dummy.write_reg_number(reg);
    uint8_t value = 0;
    dummy.read_value((uint8_t*)&value, sizeof(value));

    TEST_ASSERT_EQUAL(0x00, value);
}

void test_master_cannot_read_too_many_bytes_from_a_register() {
    size_t claimed_settings_size = 2;
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, filled_settings, claimed_settings_size, read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint32_t value = 0;
    dummy.write_reg_number(0x0);
    dummy.read_value((uint8_t*)&value, sizeof(value));

    TEST_ASSERT_EQUAL(0x0000BBAA, value);
}

void test_master_ignores_repeated_read_of_mutable_register() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, filled_settings, sizeof(filled_settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t reg_1 = 0x01;
    uint16_t value_1 = 0;
    dummy.write_reg_number(reg_1);
    dummy.read_value((uint8_t*)&value_1, sizeof(value_1));
    TEST_ASSERT_EQUAL(0xCCBB, value_1);

    // Must not populated value_2 without sending another register number first
    uint16_t value_2 = 0;
    dummy.read_value((uint8_t*)&value_2, sizeof(value_2));
    TEST_ASSERT_EQUAL(0x00, value_2);
}

void test_master_cannot_write_to_readonly_register() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), blank_read_only, sizeof(blank_read_only));
    reg_slave.listen(address);

    uint8_t reg = sizeof(settings) + 1; // First byte of read_only buffer
    uint8_t value = 0xAA;
    dummy.write_reg_number(reg);
    dummy.write_value((uint8_t*)&value, sizeof(value));

    uint8_t expected[sizeof(blank_read_only)] = {0x00, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_MEMORY(expected, blank_read_only, sizeof(blank_read_only));
}

void test_master_can_read_from_readonly_register() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint16_t value = 0;
    dummy.write_reg_number(sizeof(settings) + 2);
    dummy.read_value((uint8_t*)&value, sizeof(value));

    TEST_ASSERT_EQUAL(0x0D0C, value);
}


void test_master_can_read_from_multiple_readonly_registers() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t value_1 = 0;
    dummy.write_reg_number(sizeof(settings) + 3);
    dummy.read_value((uint8_t*)&value_1, sizeof(value_1));
    TEST_ASSERT_EQUAL(0x0D, value_1);

    uint8_t value_2 = 0;
    dummy.write_reg_number(sizeof(settings));
    dummy.read_value((uint8_t*)&value_2, sizeof(value_2));
    TEST_ASSERT_EQUAL(0x0A, value_2);
}

void test_master_cannot_read_too_many_bytes_from_a_readonly_register() {
    uint8_t long_read_only[] = {0x0A, 0x0B, 0x0C, 0x0D, 0x11, 0x11, 0x11, 0x11};
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), long_read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint32_t value = 0;
    dummy.write_reg_number(sizeof(settings) + 2);
    dummy.read_value((uint8_t*)&value, sizeof(value));

    TEST_ASSERT_EQUAL(0x00000D0C, value);
}

void test_master_ignores_repeated_read_of_readonly_register() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t reg_1 = sizeof(settings) + 0x01;
    uint16_t value_1 = 0;
    dummy.write_reg_number(reg_1);
    dummy.read_value((uint8_t*)&value_1, sizeof(value_1));
    TEST_ASSERT_EQUAL(0x0C0B, value_1);

    // Must not populated value_2 without sending another register number first
    uint16_t value_2 = 0;
    dummy.read_value((uint8_t*)&value_2, sizeof(value_2));
    TEST_ASSERT_EQUAL(0x00, value_2);
}

void test_app_receives_callback_after_read() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t callback_reg_num = 0;
    auto callback = [&callback_reg_num](uint8_t the_register) {
        callback_reg_num = the_register;
    };
    reg_slave.after_read(callback);

    uint8_t reg_1 = sizeof(settings) + 0x01;
    uint16_t value_1 = 0;
    dummy.write_reg_number(reg_1);
    dummy.read_value((uint8_t*)&value_1, sizeof(value_1));

    TEST_ASSERT_EQUAL(reg_1, callback_reg_num);
}

void test_app_receives_callback_after_write() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t callback_reg_num = 0;
    size_t callback_num_bytes = 0;
    auto callback = [&callback_reg_num, &callback_num_bytes](uint8_t the_register, size_t num_bytes) {
        callback_reg_num = the_register;
        callback_num_bytes = num_bytes;
    };
    reg_slave.after_write(callback);

    uint8_t reg = 0x0A;
    dummy.write_reg_number(reg);
    TEST_ASSERT_EQUAL(0, callback_reg_num); // Shouldn't have been called yet.

    uint16_t value = 0xFF;
    dummy.write_value((uint8_t*)&value, sizeof(value));
    TEST_ASSERT_EQUAL(reg, callback_reg_num);
    TEST_ASSERT_EQUAL(sizeof(value), callback_num_bytes);
}

void test_app_receives_callback_after_write_with_register() {
    I2CRegisterSlave reg_slave = I2CRegisterSlave(dummy, settings, sizeof(settings), read_only, sizeof(read_only));
    reg_slave.listen(address);

    uint8_t callback_reg_num = 0;
    size_t callback_num_bytes = 0;
    auto callback = [&callback_reg_num, &callback_num_bytes](uint8_t the_register, size_t num_bytes) {
        callback_reg_num = the_register;
        callback_num_bytes = num_bytes;
    };
    reg_slave.after_write(callback);

    uint8_t reg = 0x0A;
    uint16_t value = 0xFF;
    dummy.write(reg, (uint8_t*)&value, sizeof(value));
    TEST_ASSERT_EQUAL(reg, callback_reg_num);
    TEST_ASSERT_EQUAL(sizeof(value), callback_num_bytes);
}

void setup() {
    pinMode(13, OUTPUT);
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_listen_calls_listen_on_driver);

    RUN_TEST(test_master_can_write_to_mutable_register);
    RUN_TEST(test_master_can_write_to_multiple_registers);
    RUN_TEST(test_master_cannot_write_to_non_existent_register);
    RUN_TEST(test_master_cannot_write_too_many_bytes_to_a_register);
    RUN_TEST(test_master_ignores_repeated_write_to_mutable_register);

    RUN_TEST(test_master_can_write_register_in_single_transaction);
    RUN_TEST(test_master_can_write_to_multiple_registers_with_single_transactions);
    RUN_TEST(test_ignores_single_transaction_write_to_non_existent_register);
    RUN_TEST(test_master_cannot_write_too_many_bytes_to_a_register_with_single_transaction);

    RUN_TEST(test_master_can_read_from_mutable_register);
    RUN_TEST(test_master_can_read_from_multiple_mutable_registers);
    RUN_TEST(test_master_cannot_read_from_non_existent_register);
    RUN_TEST(test_master_cannot_read_too_many_bytes_from_a_register);
    RUN_TEST(test_master_ignores_repeated_read_of_mutable_register);

    RUN_TEST(test_master_cannot_write_to_readonly_register);
    RUN_TEST(test_master_can_read_from_readonly_register);
    RUN_TEST(test_master_can_read_from_multiple_readonly_registers);
    RUN_TEST(test_master_cannot_read_too_many_bytes_from_a_readonly_register);
    RUN_TEST(test_master_ignores_repeated_read_of_readonly_register);

    RUN_TEST(test_app_receives_callback_after_read);
    RUN_TEST(test_app_receives_callback_after_write);
    RUN_TEST(test_app_receives_callback_after_write_with_register);

    UNITY_END();
}

void loop() {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(900);
}

#endif //TEST_I2C_REGISTER_SLAVE