#ifndef MOTORCONTROLLER_H
#define MOTORCONTROLLER_H

/*
 * @class MotorController
 * @brief Implements the control and movement of a motor
 *
 * The MotorController class allows to set the motor,
 * Moving it, and controlling it using a simple PID controller.
*/

class MotorController {
private:
  // PWM Properties
  const int PWM_FREQUENCY = 20000; // 20 kHz
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
  float dt;

  //PID errors
  float lastError = 0;
  float integralError = 0;
  float differentialError = 0;

  const int maxIntegralError = 100;
public:

  float velSP = 0;
  float velReal = 0;
  float PWMReal = 0;

  MotorController(uint8_t pin1, uint8_t pin2, uint8_t pinEnable, float samplingTime){
    pinA = pin1;
    pinB = pin2;
    pinPWM = pinEnable;
    dt = samplingTime;
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
    } else {
      digitalWrite(pinA, LOW);
      digitalWrite(pinB, LOW);
    }

    Serial.print("Writing to pin ");
    Serial.print(pinPWM);
    Serial.print(" with ");
    Serial.println(abs(PWM));

    ledcWrite(pinPWM, abs(PWM));
  }
    
  float PID() {
    float error = velSP - velReal;

    differentialError = (error - lastError) / dt;

    integralError += error * dt; 
    integralError = constrain(integralError, -maxIntegralError, maxIntegralError);

    float correction = kp * error + ki * integralError + kd * differentialError;

    lastError = error;
    
    PWMReal = PWMReal + correction;

    return PWMReal;
  }
};

#endif
