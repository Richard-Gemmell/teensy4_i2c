// Copyright © 2021-2022 Richard Gemmell
// Released under the MIT License. See license.txt. (https://opensource.org/licenses/MIT)

#ifndef TEENSY_I2C_TESTS_EXAMPLE_TEST_SUITE_H
#define TEENSY_I2C_TESTS_EXAMPLE_TEST_SUITE_H

#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"

class ExampleTestSuite : public TestSuite {
    // We can't access the 'this' pointer in the test methods
    // so all test data has to be static.
    static int value;

public:
    void setUp() final {
        Serial.println("Set Up Example");
        value = 5;
    }

    void tearDown() final {
        Serial.println();
        Serial.println("Tear Down Example");
        value = 0;
    }

    static void test_example_one() {
        TEST_ASSERT_EQUAL(5, value);
    }

    static void test_example_two() {
        TEST_ASSERT_EQUAL(10, 1);
    }

    // Include all the tests here
    void test() final {
        RUN_TEST(test_example_one);
        RUN_TEST(test_example_two);
    }

    ExampleTestSuite() : TestSuite(__FILE__) {};
};

// Define statics
int ExampleTestSuite::value;

#endif  //TEENSY_I2C_TESTS_EXAMPLE_TEST_SUITE_H