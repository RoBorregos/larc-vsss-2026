#ifndef MOTORCONTROLLER_H
#define MOTORCONTROLLER_H

/*
 * Representation of each robot motor
*/

class MotorController {
private:
  // PWM Properties
  const int PWM_FREQUENCY = 10000;
  const int PWM_RESOLUTION = 8;
  const int MAX_PWM_VALUE = 255;

  // Motor pins
  uint8_t pinA;
  uint8_t pinB;
  uint8_t pinPWM;
  uint8_t pinEnc;

  //PID constants
  const float kp = 1;
  const float ki = 1;
  const float kd = 0;

  //PID errors
  float lastError = 0;
  float integralError = 0;
  float differentialError = 0;

public:
  MotorController(uint8_t pin1, uint8_t pin2, uint8_t pinEN, uint8_t pinEncoder) {
    pinA = pin1;
    pinB = pin2;
    pinPWM = pinEN;
    pinEnc = pinEncoder;
  }

  uint8_t get_pinEnc() {
    return pinEnc;
  }

  void setMotor() {
    // Setup positive and negative pins
    pinMode(pinA, OUTPUT);
    pinMode(pinB, OUTPUT);

    // Setup PWM pins
    ledcAttach(pinPWM, PWM_FREQUENCY, PWM_RESOLUTION);
  }
    
  float PID(int error) {
    differentialError = error - lastError;
    integralError += error;

    float correction = kp * error + ki * integralError + kd * differentialError;

    lastError = error;

    return correction;
  }
};

#endif
