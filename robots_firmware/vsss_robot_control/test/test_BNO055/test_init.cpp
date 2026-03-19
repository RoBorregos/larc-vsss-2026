#include <unity.h>
#include <Arduino.h>
#include "BNO055.hpp"

/*
    This script tests the initialization of the gyroscope and its calibration.
    It consists in letting the gyroscope still, and checking the standard deviation of the measures.
    If the standard deviation is less than a max value, the bno do not pass the test. 
*/

BNO055 bno;

const int len_data = 100;
const int max_std_dev = 0.1;

float test_gyro() {
    float arr[len_data];
    float sum = 0;
    for (int i = 0; i < len_data; i++) {
        float yaw = bno.getYaw();
        arr[i] = yaw;
        sum += yaw;
    }

    float mean = sum / len_data;

    float sum_sq = 0;

    for (int i = 0; i < len_data; i++) {
        sum_sq += pow(arr[i] - mean, 2);
    }

    float std_dev = sqrt(sum_sq /(len_data - 1));

    return std_dev;
}

void test_init() {
    TEST_ASSERT(bno.setGyro());
    TEST_ASSERT(test_gyro() < 0.1);
}