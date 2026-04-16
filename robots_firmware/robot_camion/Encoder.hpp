#ifndef ENCODER_HPP
#define ENCODER_HPP

#include "driver/pulse_cnt.h"
#include "esp_timer.h"

class Encoder {

private:
    gpio_num_t pin_;
    pcnt_unit_handle_t unit_;
    pcnt_channel_handle_t channel_;

    int high_limit_ = 32767;
    int low_limit_  = -32768;

    float ppr_;
    int direction_ = 1;

    int64_t last_time_us_ = 0;

    // Low-pass filter variables
    float rpm_filtered_ = 0.0f;
    float alpha_ = 0.3f;   // ajustar según necesidad
    float final_rps = 0.0f;
public:
    Encoder(gpio_num_t pin, float ppr)
        : pin_(pin), ppr_(ppr) {}

    void begin() {
        gpio_set_direction(pin_, GPIO_MODE_INPUT);

        pcnt_unit_config_t unit_config = {
            .low_limit  = low_limit_,
            .high_limit = high_limit_
        };

        ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &unit_));

        pcnt_chan_config_t chan_config = {
            .edge_gpio_num  = pin_,
            .level_gpio_num = -1
        };

        ESP_ERROR_CHECK(pcnt_new_channel(unit_, &chan_config, &channel_));

        pcnt_channel_set_edge_action(
            channel_,
            PCNT_CHANNEL_EDGE_ACTION_INCREASE,
            PCNT_CHANNEL_EDGE_ACTION_HOLD
        );

        pcnt_glitch_filter_config_t filter_config = {
            .max_glitch_ns = 5000
        };

        pcnt_unit_set_glitch_filter(unit_, &filter_config);

        pcnt_unit_enable(unit_);
        pcnt_unit_clear_count(unit_);
        pcnt_unit_start(unit_);

        last_time_us_ = esp_timer_get_time();
    }

    void set_direction(int direction) {
        direction_ = (direction >= 0) ? 1 : -1;
    }

    void update() {
        int64_t now_us  = esp_timer_get_time();
        int64_t elapsed = now_us - last_time_us_;

        if (elapsed <= 0) return;

        int pulse_count = 0;

        pcnt_unit_get_count(unit_, &pulse_count);
        pcnt_unit_clear_count(unit_);

        last_time_us_ = now_us;

        float dt_seconds = elapsed * 1e-6f;
        if (dt_seconds == 0) return;

        float rpm_raw =
            (static_cast<float>(pulse_count) * 60.0f) /
            (ppr_ * dt_seconds);

        rpm_raw *= direction_;

        // Low-pass filter exponencial
        rpm_filtered_ =
            alpha_ * rpm_raw +
            (1.0f - alpha_) * rpm_filtered_;

        final_rps = rpm_filtered_ / 60;
    }

    float get_rps() {
        return final_rps;
    }

    void reset() {
        pcnt_unit_clear_count(unit_);

        last_time_us_ = esp_timer_get_time();

        rpm_filtered_ = 0.0f;
    }

    void setFilterAlpha(float alpha) {
        if (alpha > 0.0f && alpha <= 1.0f)
            alpha_ = alpha;
    }
};

#endif