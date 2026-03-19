#include <unity.h>
#include "Communication.hpp"

#include "config.hpp"

Communication udp {SSID, PASSWORD, 8081};

bool message_received() {

    udp.configComms();

    robotCmdPacket cmd;

    double limit_time = 10000;

    double t0 = millis();
    for ( ;; ) {
        bool received = udp.udpReceiveCmd(&cmd);

        double tn = millis();

        if (received) {
            return true;
        }
        if (tn - t0 > limit_time) {
            return false;
        }
    }
}

void test_connection() {
    TEST_ASSERT(message_received());
}