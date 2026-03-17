#include "MotorController.hpp"

MotorController::MotorController(uint8_t in1, uint8_t in2, uint8_t PWM, uint8_t PWM_channel, 
                                uint8_t enc1, uint8_t enc2, pcnt_unit_t unit, float sampling_time)
    : in1_(in1), in2_(in2), PWM_(PWM), PWM_channel_(PWM_channel), 
    enc1_(enc1), enc2_(enc2), pulse_counter_(unit), SAMPLING_TIME_(sampling_time) 
    {
        setMotor();
    } 

void MotorController::setMotor() {
    pinMode(in1_, OUTPUT);
    pinMode(in2_, OUTPUT);

    ledcSetup(PWM_channel_, PWM_FREQUENCY_, PWM_RESOLUTION_);
    ledcAttachPin(PWM_, PWM_channel_);

    pcnt_config_t pcnt_config = {};
    // Channel 0
    pcnt_config.pulse_gpio_num = enc1_;
    pcnt_config.ctrl_gpio_num  = enc2_;
    pcnt_config.channel = PCNT_CHANNEL_0;
    pcnt_config.unit = pulse_counter_;

    pcnt_config.pos_mode = PCNT_COUNT_INC;
    pcnt_config.neg_mode = PCNT_COUNT_DEC;

    pcnt_config.lctrl_mode = PCNT_MODE_REVERSE;
    pcnt_config.hctrl_mode = PCNT_MODE_KEEP;

    pcnt_config.counter_h_lim = 32767;
    pcnt_config.counter_l_lim = -32768;

    pcnt_unit_config(&pcnt_config);


    // Channel 1
    pcnt_config.pulse_gpio_num = enc2_;
    pcnt_config.ctrl_gpio_num  = enc1_;
    pcnt_config.channel = PCNT_CHANNEL_1;

    pcnt_config.pos_mode = PCNT_COUNT_DEC;
    pcnt_config.neg_mode = PCNT_COUNT_INC;

    pcnt_config.lctrl_mode = PCNT_MODE_REVERSE;
    pcnt_config.hctrl_mode = PCNT_MODE_KEEP;

    pcnt_unit_config(&pcnt_config);

    pcnt_counter_pause(pulse_counter_);
    pcnt_counter_clear(pulse_counter_);

    pcnt_counter_resume(pulse_counter_);
}

void MotorController::newSetpoint(float sp) {
    setpoint_ = sp;
}

void MotorController::readEncoder() {
    int16_t count;

    pcnt_get_counter_value(pulse_counter_, &count);
    pcnt_counter_clear(pulse_counter_);

    float vel = (count * 60) / (ENC_RESOLUTION_ * SAMPLING_TIME_);
    rpm_lecture_ = vel;
}

float MotorController::pid() {
    float error = setpoint_ - rpm_lecture_;

    differential_error_ = (error - last_error_) / SAMPLING_TIME_;
    integral_error_ += error * SAMPLING_TIME_;
    integral_error_ = constrain(integral_error_, -MAX_INTEGRAL_ERROR_, MAX_INTEGRAL_ERROR_);

    float correction = kp_ * error + ki_ * differential_error_ + kd_ * integral_error_;

    last_error_ = error;  

    correction += last_correction_;

    last_correction_ = correction;

    return correction;
}

void MotorController::move(float pwm) {
    pwm = constrain(pwm, -255, 255);

    if (pwm > 0) {
        digitalWrite(in1_, HIGH);
        digitalWrite(in2_, LOW);
        ledcWrite(PWM_channel_, abs(pwm));
    } else if (pwm < 0) {
        digitalWrite(in1_, LOW);
        digitalWrite(in2_, HIGH);
        ledcWrite(PWM_channel_, abs(pwm));
    } else {
        digitalWrite(in1_, LOW);
        digitalWrite(in2_, LOW);
        ledcWrite(PWM_channel_, abs(pwm));
    }

}