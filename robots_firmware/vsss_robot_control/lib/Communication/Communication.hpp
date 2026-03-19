#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <WiFi.h>
#include <WiFiUdp.h>

struct __attribute__((packed)) robotInitPacket {
    int msg;
};

struct __attribute__((packed)) robotCmdPacket {
    float rpm_left;
    float rpm_right;
    float rpm_back;
    bool kicker;
    bool stop;
};

struct __attribute__((packed)) robotStatePacket {
    float yaw;
    float rpm_left;
    float rpm_right;
    float rpm_back;
};

class Communication {
private:
    static constexpr int initPacketSize_ = sizeof(robotInitPacket);
    static constexpr int cmdPacketSize_ = sizeof(robotCmdPacket);
    static constexpr int statePacketSize_ = sizeof(robotStatePacket);

    const char *ssid_;
    const char *password_;
    int local_port_;

    WiFiUDP udp_;
public:
    Communication(const char *ssid, const char *password, int local_port);

    wl_status_t configComms();
    bool udpReceiveCmd(robotCmdPacket *cmd);
    void udpSendMessage(float yaw, float rpm_left, float rpm_right, float rpm_back);
};

#endif