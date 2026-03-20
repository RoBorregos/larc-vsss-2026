#include "utils.hpp"

double degToRad(double deg) {
    double rad = deg * M_PI / 180.0;

    rad = fmod(rad + M_PI, 2 * M_PI);
    if (rad < 0) {
        rad += 2 * M_PI;
    }

    return rad - M_PI;
}
