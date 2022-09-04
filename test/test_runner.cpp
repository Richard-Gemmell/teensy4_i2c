#include <unity.h>
#include <Arduino.h>
#include "utils/test_suite.h"

// Unit Tests
//#include "example/example.h"
#include "unit/test_i2c_device.h"
#include "unit/test_i2c_register_slave.h"

// End-to-End Tests
#include "e2e/loopback/bus_trace/test_bus_recorder.h"
#include "e2e/loopback/driver_config/test_pullup_config.h"
#include "e2e/loopback/driver_config/test_pullup_config_wire.h"
#include "e2e/loopback/logic/test_e2e_basic_messages.h"
//#include "e2e/loopback/signals/test_e2e_slave_signals.h"
//#include "e2e/loopback/signals/test_e2e_master_signals.h"
#include "e2e/loopback/signals/test_e2e_rise_times.h"
#include "e2e/loopback/wire/test_e2e_driver_wire.h"

void test(TestSuite* suite);

// Runs a limited number of test suites.
// Return true to run all tests afterwards.
bool run_subset() {
    return true;
//    test(new e2e::loopback::logic::BasicMessagesTest());
//    test(new e2e::loopback::signals::MasterSignalsTest());
//    return false;
}

// Runs every test suite in succession.
void run_all_tests() {
    // Unit tests run an any board layout
    Serial.println("Run Unit Tests");
    Serial.println("--------------");
//    test(new ExampleTestSuite());
    test(new I2CDeviceTest());
    test(new I2CRegisterSlaveTest());

    // Full Stack Tests
    // These tests require working hardware
    Serial.println("Run Full Stack (E2E) Tests");
    Serial.println("--------------------");
    test(new e2e::loopback::bus_trace_tests::BusRecorderTest());
    test(new e2e::loopback::driver_config::PullupConfigTest());
    test(new e2e::loopback::driver_config::PullupConfigWireTest());
    test(new e2e::loopback::logic::BasicMessagesTest());
//    test(new e2e::loopback::signals::MasterSignalsTest());
    test(new e2e::loopback::signals::RiseTimeTest());
//    test(new e2e::loopback::signals::SlaveSignalTest());
    test(new e2e::loopback::wire::DriverWireTest());
}

TestSuite* test_suite;
void report_test_results();
UNITY_COUNTER_TYPE tests_run = 0;
UNITY_COUNTER_TYPE tests_ignored = 0;
UNITY_COUNTER_TYPE tests_failed = 0;

void test(TestSuite* suite) {
    test_suite = suite;
    UnitySetTestFile(test_suite->get_file_name());
    test_suite->test();
    delete(test_suite);
    tests_run = Unity.NumberOfTests - tests_run;
    tests_failed = Unity.TestFailures - tests_failed;
    tests_ignored = Unity.TestIgnores - tests_ignored;
    report_test_results();
    Serial.println("");
}

// Called before each test.
__attribute__((unused)) void setUp(void) {
    test_suite->setUp();
}

// Called after each test.
__attribute__((unused)) void tearDown(void) {
    test_suite->tearDown();
}

// Blink the LED to make sure the Teensy hasn't hung
IntervalTimer blink_timer;
void blink_isr();

void setup() {
    // Blink the LED to show we're still alive
    blink_timer.begin(blink_isr, 300'000);

    // You must connect the Serial Monitor before this delay times out
    // otherwise the test output gets truncated in a weird way.
    delay(1000);
    UNITY_BEGIN();
    if(run_subset()) {
        run_all_tests();
    }
    Serial.flush();
    delay(100);
    UNITY_END();
}

int max_pokes = 10;
void loop() {
    delay(1000);
    if (max_pokes-- > 0) {
        // Print a character from time to time to force the serial monitor
        // to refresh in case the user didn't connect the monitor in time.
        Serial.print(" ");
    }
}

// Equivalent to UNITY_END() except it doesn't halt the test runner.
void report_test_results() {
    Serial.println("----------------------");
    Serial.print(tests_run);
    Serial.print(" Executed "); // DON'T say "Tests" here. It'll stop the test suite running.
    Serial.print(tests_failed);
    Serial.print(" Failures ");
    Serial.print(tests_ignored);
    Serial.println(" Ignored ");
    if (tests_failed == 0U) {
        Serial.println("OK");
    } else {
        Serial.println("FAIL");
    }
    Serial.println(".");
}

void blink_isr() {
    digitalToggle(LED_BUILTIN);
}
