#ifndef MOTOR_HPP
#define MOTOR_HPP

#include "Constants.hpp"
#include "Encoder.hpp"
#include "PID.h"

namespace {
	float clamp(float value, float upper, float lower) {
    return min(upper, max(value, lower));
  }
}

class Motor {
private:
	const int PWM_FREQUENCY = 20000;
	const int PWM_RESOLUTION = 8;
	
	uint8_t in_a;
	uint8_t in_b;
	uint8_t in_pwm;

	PID::PidParameters pid_parameters;

	unsigned long long previous_tick = 0;
public:
	Encoder enc;

	Motor(uint8_t in_a, uint8_t in_b, uint8_t in_pwm, gpio_num_t pinENC): enc(pinENC, ENCODER_RESOLUTION) {
		this->in_a = in_a;
		this->in_b = in_b;
		this->in_pwm = in_pwm;

		pid_parameters.kp = 0;
	  pid_parameters.ki = 0;
	  pid_parameters.kd = 0;

	  pid_parameters.one_direction_only = true;
	  pid_parameters.max_output = MAX_PWM_VALUE;
	  pid_parameters.error_threshold = 0.0;
	}

	void begin() {
		pinMode(in_a, OUTPUT);
		pinMode(in_b, OUTPUT);
		ledcAttach(in_pwm, PWM_FREQUENCY, PWM_RESOLUTION);
	}

	void tick() {
		if (millis() - previous_tick >= 10) {
			enc.update();

			previous_tick = millis();
		}
	}

	void set_pwm(int pwm_value) {
		int pwm = clamp(pwm_value, -MAX_PWM_VALUE, MAX_PWM_VALUE);
		enc.set_direction(pwm_value);

		bool forward = pwm > 0;
		bool backward = pwm < 0;

		digitalWrite(in_a, forward);
		digitalWrite(in_b, backward);

		ledcWrite(in_pwm, pwm);
	}

	void move(float new_rps) {
		if (fabs(new_rps) <= 0.01) {
			set_pwm(0);
			return;
		}

		float current_rps = enc.get_rps();
		pid_parameters.error = new_rps - current_rps;
		pid_parameters.target = new_rps;

		if (new_rps > 0) pid_parameters.accept_type = PID_ACCEPT_POSITIVES_ONLY;
		else pid_parameters.accept_type = PID_ACCEPT_NEGATIVES_ONLY;

		PID::compute(pid_parameters);

		set_pwm(static_cast<float>(pid_parameters.output));
	}

	void brake() {
		digitalWrite(in_a, HIGH);
		digitalWrite(in_b, HIGH);

		PID::reset(pid_parameters);

		ledcWrite(in_pwm, 0);
	}

	void print_debug() {
		Serial.print(millis()); Serial.print(": ");
		Serial.print(pid_parameters.output); Serial.print(", ");
		Serial.print(enc.get_rps()); Serial.print(", ");
		Serial.println(pid_parameters.integral_error);
	}
};

#endif
