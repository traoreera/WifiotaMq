#include <Arduino.h>
#include <unity.h>
#include "../src/utilities.h" // Include the utilities.h from the library

Logger test_logger;

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_logger_info_message() {
    test_logger.setLevel(Logger::INFO);
    test_logger.isEnabled_logger = true;
    // Redirect Serial output for testing if possible, otherwise rely on visual inspection or mock
    // For this basic test, we'll just ensure the call doesn't crash and conceptually works.
    test_logger.info("This is an info message.");
    TEST_PASS(); // If it reaches here, it hasn't crashed, which is a start
}

void test_logger_debug_message_disabled() {
    test_logger.setLevel(Logger::INFO); // Debug messages should be filtered out
    test_logger.isEnabled_logger = true;
    test_logger.debug("This is a debug message.");
    TEST_PASS(); // Expect it not to crash
}

void setup() {
    // NOTE: C++ `main` is replaced by `setup` and `loop` in Arduino.
    // However, for platformio unit tests, `UNITY_BEGIN()` is often called in `setup`.
    delay(2000); // Wait for serial monitor to open

    UNITY_BEGIN();

    RUN_TEST(test_logger_info_message);
    RUN_TEST(test_logger_debug_message_disabled);

    UNITY_END(); // stop unit testing
}

void loop() {
    // Nothing to do here.
}
