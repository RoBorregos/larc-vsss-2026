#include "MPU6050.hpp"

MPU6050::MPU6050(uint8_t sda, uint8_t scl, GyroRange range)
  : sda_(sda), scl_(scl), range_(range) {}

bool MPU6050::begin() {
  Wire.begin(sda_, scl_);
  Wire.setClock(100000);
  delay(250);

  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() != 0) return false;

  writeReg(PWR_MGMT_1, 0x00);  
  delay(100);
  writeReg(GYRO_CONFIG, static_cast<uint8_t>(range_));
  delay(100);

  last_time_ = micros();
  return true;
}

void MPU6050::calibrate(int samples) {
  float sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += readGyroZ();
    delay(5);
  }
  gyro_bias_z_ = sum / samples;
}

void MPU6050::update() {
  unsigned long now = micros();
  float dt = (now - last_time_) * micToSec;
  last_time_ = now;

  int16_t raw   = readGyroZ();
  float gyro_z  = ((raw - gyro_bias_z_) / getScale()) * DEG_TO_RAD;

  if (fabs(gyro_z) < ZERO_THRESHOLD) gyro_z = 0.0f;

  gyro_z = movingAverage(gyro_z);

  yaw_ += gyro_z * dt;
  yaw_  = atan2(sin(yaw_), cos(yaw_));
}


void MPU6050::writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

int16_t MPU6050::readGyroZ() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(GYRO_ZOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t) 2);
  return (Wire.read() << 8) | Wire.read();
}

float MPU6050::getScale() const {
  switch (range_) {
    case GyroRange::RANGE_250:  return 131.0f;
    case GyroRange::RANGE_500:  return 65.5f;
    case GyroRange::RANGE_1000: return 32.8f;
    case GyroRange::RANGE_2000: return 16.4f;
    default:                    return 131.0f;
  }
}

float MPU6050::movingAverage(float val) {
  gyro_buffer_[buf_index_] = val;
  buf_index_ = (buf_index_ + 1) % NUM_SAMPLES;
  float sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) sum += gyro_buffer_[i];
  return sum / NUM_SAMPLES;
}
