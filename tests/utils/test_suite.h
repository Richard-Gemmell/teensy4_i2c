// Copyright © 2021-2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY_I2C_TESTS_TEST_SUITE_H
#define TEENSY_I2C_TESTS_TEST_SUITE_H

class TestSuite {
public:
    explicit TestSuite(const char* test_file_name)
        : test_file_name(test_file_name) {
    }

    virtual ~TestSuite() = default;

    // Called before each test
    virtual void setUp() {};

    // Called after each test
    virtual void tearDown() {};

    // Return the name of the test file
    const char* get_file_name() {
        return test_file_name;
    };

    // Executes the tests in the suite
    virtual void test() = 0;

private:
    const char* test_file_name;
};

#endif //TEENSY_I2C_TESTS_TEST_SUITE_H
