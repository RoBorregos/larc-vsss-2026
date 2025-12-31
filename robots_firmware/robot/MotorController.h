#ifndef MOTORCONTROLLER_H
#define MOTORCONTROLLER_H

/*
 * Representation of each robot motor
*/

class MotorController {
private:
  // PWM Properties
  const int PWM_FREQUENCY = 5000; // 5 kHz
  const int PWM_RESOLUTION = 8; // 8-bit resolution (0-255)
  const int MAX_PWM_VALUE = 255;

  // Motor pins
  uint8_t pinA;
  uint8_t pinB;
  uint8_t pinPWM;

  //PID constants
  const float kp = 1;
  const float ki = 1;
  const float kd = 0;
  const float dt = 0.005;

  //PID errors
  float lastError = 0;
  float integralError = 0;
  float differentialError = 0;
public:

  float velSP = 0;
  
  MotorController(uint8_t pin1, uint8_t pin2, uint8_t pinEnable){
    pinA = pin1;
    pinB = pin2;
    pinPWM = pinEnable;
  }

  void setMotor() {
    pinMode(pinA, OUTPUT);
    pinMode(pinB, OUTPUT);
    ledcAttach(pinPWM, PWM_FREQUENCY, PWM_RESOLUTION);
  }

  void move(int PWM) {
    PWM = constrain(PWM, -MAX_PWM_VALUE, MAX_PWM_VALUE);

    if (PWM > 0) {
      digitalWrite(pinA, HIGH);
      digitalWrite(pinB, LOW);
    } else if (PWM < 0) {
      digitalWrite(pinA, LOW);
      digitalWrite(pinB, HIGH);
    }

    ledcWrite(pinPWM, abs(PWM));
  }
    
  float PID(int error) {
    differentialError = error - lastError;
    integralError += error;

    float correction = kp * error + ki * integralError * dt + kd * differentialError / dt;

    lastError = error;

    return correction;
  }
};

#endif
