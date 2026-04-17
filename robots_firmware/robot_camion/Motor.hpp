#ifndef MOTOR_HPP
#define MOTOR_HPP

#include "Constants.hpp"
#include "Encoder.hpp"
#include "PID.h"

namespace {
	float clamp(float value, float lower, float upper) {
    return min(upper, max(value, lower));
  }
}

class Motor {
private:
	const int PWM_FREQUENCY = 20000;
	const int PWM_RESOLUTION = 8;

	int id;
	uint8_t in_a;
	uint8_t in_b;
	uint8_t in_pwm;

	PID::PidParameters pid_parameters;

	unsigned long long previous_tick = 0;
public:
	Encoder enc;

	Motor(int id, uint8_t in_a, uint8_t in_b, uint8_t in_pwm, gpio_num_t pinENC): enc(pinENC, ENCODER_RESOLUTION) {
		this->id = id;
		this->in_a = in_a;
		this->in_b = in_b;
		this->in_pwm = in_pwm;

	  pid_parameters.one_direction_only = true;
	  pid_parameters.error_threshold = 0.0;
	}

	void begin() {
		pinMode(in_a, OUTPUT);
		pinMode(in_b, OUTPUT);
		ledcAttach(in_pwm, PWM_FREQUENCY, PWM_RESOLUTION);

		digitalWrite(in_a, LOW);
		digitalWrite(in_b, LOW);
		enc.begin();
	}

	void attach_kterms(float kp, float ki, float kd) {
		pid_parameters.kp = kp;
	  pid_parameters.ki = ki;
	  pid_parameters.kd = kd;
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

		if (pwm_value > 0) {
			digitalWrite(in_a, HIGH);
			digitalWrite(in_b, LOW);			
		} else if (pwm_value < 0) {
			digitalWrite(in_a, LOW);
			digitalWrite(in_b, HIGH);			
		} else {
			digitalWrite(in_a, LOW);
			digitalWrite(in_b, LOW);
		}

		ledcWrite(in_pwm, abs(pwm));
	}

	void move(float new_rps) {
		if (fabs(new_rps) <= 0.01) {
			set_pwm(0);
			PID::reset(pid_parameters);
			return;
		}

		float current_rps = enc.get_rps();

		pid_parameters.error = new_rps - current_rps;
		pid_parameters.target = new_rps;

		if (new_rps > 0) pid_parameters.accept_type = PID_ACCEPT_POSITIVES_ONLY;
		else pid_parameters.accept_type = PID_ACCEPT_NEGATIVES_ONLY;

		PID::compute(pid_parameters);

		// Serial.print("target: "); Serial.print(pid_parameters.target);
		// Serial.print(" - err: "); Serial.print(pid_parameters.error);
		// Serial.print(" - out: "); Serial.println(pid_parameters.output);

		set_pwm(static_cast<float>(pid_parameters.output));
	}

	void brake() {
		digitalWrite(in_a, HIGH);
		digitalWrite(in_b, HIGH);

		PID::reset(pid_parameters);

		ledcWrite(in_pwm, 0);
	}

	void print_debug() {
		Serial.print(id); Serial.print(" - ");

		Serial.print(pid_parameters.output); Serial.print(", ");
		Serial.print(enc.get_rps()); Serial.print(", ");
		Serial.print(pid_parameters.integral_error); Serial.print(", ");
		Serial.print(pid_parameters.error); Serial.print(", ");
		Serial.println(pid_parameters.target);
	}
};

#endif
