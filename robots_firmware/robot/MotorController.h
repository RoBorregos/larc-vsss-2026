#ifndef MOTORCONTROLLER_H
#define MOTORCONTROLLER_H

/*
 * Representation of each robot motor
*/

class MotorController {
private:

  //PID constants
  const float kp = 1;
  const float ki = 1;
  const float kd = 0;
  const float dt = 0.0005;

  //PID errors
  float lastError = 0;
  float integralError = 0;
  float differentialError = 0;

public:
  MotorController(){}
    
  float PID(int error) {
    differentialError = error - lastError;
    integralError += error;

    float correction = kp * error + ki * integralError * dt + kd * differentialError / dt;

    lastError = error;

    return correction;
  }
};

#endif
