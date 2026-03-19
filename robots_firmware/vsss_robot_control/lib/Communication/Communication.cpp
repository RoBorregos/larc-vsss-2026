#include "Communication.hpp"

Communication::Communication(const char *ssid, const char *password, int local_port)
    : ssid_(ssid), password_(password), local_port_(local_port) {}

wl_status_t Communication::configComms() {
    // Connect to WiFi
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid_);
    WiFi.begin(ssid_, password_);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Start UDP server
    udp_.begin(local_port_);
    Serial.print("UDP server started on port ");
    Serial.println(local_port_);

    return WiFi.status();
}


bool Communication::udpReceiveCmd(robotCmdPacket *cmd) {
    int packet = udp_.parsePacket();
    if (packet == cmdPacketSize_) {

        robotCmdPacket cmd;

        udp_.read((uint8_t*) &cmd, cmdPacketSize_);

        return true;
    } else if (packet > 0) {
        Serial.print("bad packet");
        udp_.flush();
    }
    return false;
}

void Communication::udpSendMessage(float yaw, float rpm_left, float rpm_right, float rpm_back) {
    robotStatePacket state;

    state.yaw = yaw;
    state.rpm_left = rpm_left;
    state.rpm_right = rpm_right;
    state.rpm_back = rpm_back;

    udp_.beginPacket(udp_.remoteIP(), udp_.remotePort());
    udp_.write((uint8_t*)&state, statePacketSize_);
    udp_.endPacket();
}