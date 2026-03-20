#include <unity.h>
#include <Arduino.h>

void test_init();
void test_angle();

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_init);
    RUN_TEST(test_angle);
    UNITY_END();
}

void loop() {

}