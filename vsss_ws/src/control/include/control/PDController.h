#ifndef PDController_H
#define PDController_H

class PDController {
private:
    double kp; // Proportional gain
    double kd; // Derivative gain
    double previous_error; // To store the previous error value
    double dt; // Time step
public:
    PDController(double kp, double kd)
        : kp(kp), kd(kd), previous_error(0.0), dt(dt) {};
    double update(double error);
};

#endif // PDController_H




