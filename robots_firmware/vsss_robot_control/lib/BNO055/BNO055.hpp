#ifndef BNO055_HPP
#define BNO055_HPP

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <utility/imumaths.h>
#include "utils.hpp"

class BNO055 {
private:
    static constexpr float BNO055_SAMPLE_RATE = 100;

    uint8_t SDA_;
    uint8_t SCL_;
    Adafruit_BNO055 bno_ = Adafruit_BNO055(55);
public:
    BNO055();

    BNO055(uint8_t SDA, uint8_t SCL);

    bool setGyro();    

    double getYaw();
};

#endif
