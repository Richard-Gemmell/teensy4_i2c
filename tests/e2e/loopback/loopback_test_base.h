// Copyright (c) 2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY4_I2C_LOOPBACK_TEST_BASE_H
#define TEENSY4_I2C_LOOPBACK_TEST_BASE_H

#include "e2e/e2e_test_base.h"
#include "e2e/loopback/loopback.h"

namespace e2e {
namespace loopback {

class LoopbackTestBase : public e2e::E2ETestBase {
public:
    explicit LoopbackTestBase(const char* test_file_name)
        : e2e::E2ETestBase(test_file_name) {
    }

    void setUp() override {
        loopback::Loopback::disable_all_pullups();
        E2ETestBase::setUp();
    }

    void tearDown() override {
        E2ETestBase::tearDown();
        Master.end();
        Slave.stop_listening();
        Master1.end();
        Slave1.stop_listening();
        Master2.end();
        Slave2.stop_listening();
        loopback::Loopback::disable_all_pullups();
    }
};

}
}

#endif //TEENSY4_I2C_LOOPBACK_TEST_BASE_H
