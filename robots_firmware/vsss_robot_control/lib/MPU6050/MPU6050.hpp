#ifndef MPU6050_HPP
#define MPU6050_HPP

#include <Arduino.h>
#include <Wire.h>
#include "utils.hpp"

enum class GyroRange {
  RANGE_250  = 0x00,  // 131.0 LSB/°/s
  RANGE_500  = 0x08,  // 65.5  LSB/°/s
  RANGE_1000 = 0x10,  // 32.8  LSB/°/s
  RANGE_2000 = 0x18   // 16.4  LSB/°/s
};

class MPU6050 {
public:
  MPU6050(uint8_t sda, uint8_t scl, GyroRange range = GyroRange::RANGE_250);
  bool begin();
  void calibrate(int samples = 500);
  void update();
  float getYawRad() const { return yaw_; }
  float getYawDeg() const { return yaw_ * RAD_TO_DEG; }
  void reset() { yaw_ = 0; }

private:
  uint8_t sda_;
  uint8_t scl_;
  GyroRange range_;

  float gyro_bias_z_ = 0;
  float yaw_         = 0;
  unsigned long last_time_ = 0;

  static constexpr uint8_t MPU_ADDR    = 0x68;
  static constexpr uint8_t PWR_MGMT_1  = 0x6B;
  static constexpr uint8_t GYRO_CONFIG = 0x1B;
  static constexpr uint8_t GYRO_ZOUT_H = 0x47;

  static constexpr uint8_t  NUM_SAMPLES    = 5;
  static constexpr float    ZERO_THRESHOLD = 0.01f;

  float gyro_buffer_[NUM_SAMPLES] = {0};
  int   buf_index_ = 0;

  void  writeReg(uint8_t reg, uint8_t val);
  int16_t readGyroZ();
  float getScale() const;
  float movingAverage(float val);
};

#endif
