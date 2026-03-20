#include "BNO055.hpp"

BNO055::BNO055() {
    Wire.begin();
};

BNO055::BNO055(uint8_t SDA, uint8_t SCL) 
    :   SDA_(SDA), SCL_(SCL) {
        Wire.begin(SDA_, SCL_);
};

bool BNO055::setGyro() {
    if (!bno_.begin()) {
        return false;
    }

    bno_.setExtCrystalUse(true);

    return true;
}

double BNO055::getYaw() {
    sensors_event_t event;
    bno_.getEvent(&event);
    float yaw = event.orientation.x;

    yaw = degToRad(yaw);

    return yaw;
}