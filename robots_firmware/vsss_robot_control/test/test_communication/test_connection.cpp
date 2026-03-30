#include <unity.h>
#include "Communication.hpp"

#include "config.hpp"

Communication test_udp_1 {Config::SSID, Config::PASSWORD, 8081};

void test_connection() {
    TEST_ASSERT(test_udp_1.configComms());
    TEST_ASSERT(test_udp_1.commsInit());
}