#ifndef MOTORCONTROLlER_H
#define MOTORCONTROLLER_H

#include <Arduino.h>
#include <driver/pcnt.h>

class MotorController {
private:
    static constexpr int PWM_FREQUENCY_ = 5000;
    static constexpr int PWM_RESOLUTION_ = 8;
    static constexpr int MAX_PWM_VALUE_ = 255;
    static constexpr float ENC_RESOLUTION_ = 4 * 350;

    float SAMPLING_TIME_ = 0.02;
    float MAX_INTEGRAL_ERROR_ = 1000;

    uint8_t in1_;
    uint8_t in2_;
    uint8_t PWM_;
    uint8_t PWM_channel_;
    uint8_t enc1_;
    uint8_t enc2_;
    pcnt_unit_t pulse_counter_;

    float setpoint_ = 20;

    float kp_ = 1;
    float ki_ = 0;
    float kd_ = 0;

    float last_error_ = 0;
    float integral_error_ = 0;
    float differential_error_ = 0;

    float last_correction_ = 0;

    bool first_interation_ = true;

public:
    MotorController(uint8_t in1, uint8_t in2, uint8_t PWM, uint8_t PWM_channel, 
                    uint8_t enc1, uint8_t enc2, pcnt_unit_t unit, float sampling_time_);

    void setMotor();
    void newSetpoint(float sp);
    float readEncoder();
    float pid(float rpm);
    void move(float pwm);
    void stop();
};

#endif