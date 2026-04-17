/*
 * MIT License
 * Copyright (c) 2025 PrepaTec Eugenio Garza Lagüera AIZER
 * 
 Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

https://github.com/aizer-egl/SOCCER_OPEN_2025
 */

#ifndef PICO_LIB_PID_H
#define PICO_LIB_PID_H

#include <Arduino.h>

#define PID_ACCEPT_POSITIVES_ONLY true
#define PID_ACCEPT_NEGATIVES_ONLY false

class PID {
public:
    struct PidParameters {
        float kp{};
        float ki{};
        float kd{};

        double max_output = 0.0;
        double min_output = 0.0;
        float error_threshold = 0.0;

        double integral_error = 0.0;
        double previous_error = 0.0;
        unsigned long long last_iteration_ms = 0;

        bool first_run = true;

        double target = 0.0;
        double error = 0.0;
        double output = 0.0;

        // IF one_direction_only SET TO TRUE
        // it will only accept the values specified in the accept_positives
        bool one_direction_only = false;
        bool accept_type = PID_ACCEPT_POSITIVES_ONLY;

        // IF reset_within_threshold SET TO TRUE
        // it will execute PID::reset if within error_threshold
        bool reset_within_threshold = false;
    };
    
    static void reset(PidParameters& pid);
    static void compute(PidParameters& pid);
};


#endif //PICO_LIB_PID_H
