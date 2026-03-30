#include <unity.h>
#include "Communication.hpp"

#include "config.hpp"

Communication test_udp_2 {Config::SSID, Config::PASSWORD, 8081};

bool message_received() {

    test_udp_2.configComms();

    robotCmdPacket cmd;

    double limit_time = 10000;

    double t0 = millis();
    for ( ;; ) {
        bool received = test_udp_2.udpReceiveCmd(&cmd);

        double tn = millis();

        if (received) {
            return true;
        }
        if (tn - t0 > limit_time) {
            return false;
        }
    }
}

void test_communication() {
    TEST_ASSERT(message_received());
}