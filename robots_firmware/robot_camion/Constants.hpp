#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

inline constexpr int NUM_MOTORS = 3;
inline constexpr float ENCODER_RESOLUTION = 826.0f;
inline constexpr float SAMPLING_TIME = 0.02f;
inline constexpr float MAX_PWM_VALUE = 255;

inline constexpr uint8_t inA1 = 9;
inline constexpr uint8_t inA2 = 14;
inline constexpr uint8_t PWMA = 8;
inline constexpr gpio_num_t ENCA = GPIO_NUM_7;

inline constexpr uint8_t inB1 = 19;
inline constexpr uint8_t inB2 = 18;
inline constexpr uint8_t PWMB = 20;
inline constexpr gpio_num_t ENCB = GPIO_NUM_6;

inline constexpr uint8_t inC1 = 2;
inline constexpr uint8_t inC2 = 1;
inline constexpr uint8_t PWMC = 0;
inline constexpr gpio_num_t ENCC = GPIO_NUM_5;

#endif
