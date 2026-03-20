#include <unity.h>
#include <Arduino.h>
#include "BNO055.hpp"

/*
    This test checks if the bno gets out of bounds.
    It gets different measures and check if the value is less than -pi or more the pi
*/

BNO055 bno1;

bool test_bno() {
    unsigned long t0 = millis();
    unsigned long tn = millis();
    while (tn - t0 < 20000) {
        double yaw = bno1.getYaw();
        if (yaw > M_PI || yaw < - M_PI){
            return false;
        }
        delay(10);
        tn = millis();
    }
    return true;
}

void test_angle() {
    TEST_ASSERT(bno1.setGyro());
    TEST_ASSERT(test_bno());
}
