#include <Arduino.h>
#include <unity.h>

void test_connection();
void test_communication();

void setup() {
    delay(2000);
    UNITY_BEGIN();

    RUN_TEST(test_connection);

    UNITY_END();
}

void loop() {}