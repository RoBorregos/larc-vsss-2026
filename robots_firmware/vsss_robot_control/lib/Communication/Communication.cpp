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

bool Communication::commsInit() {
    Serial.println("Waiting for handshake...");
    bool connected = false;
    int pingNumber = 50056;
    int pongNumber = 50057;
    
    while (!connected) {
        int packetSize = udp_.parsePacket();
        if (packetSize == sizeof(robotInitPacket)) {
            robotInitPacket ping;
            udp_.read((uint8_t*)&ping, sizeof(ping));

            if (ping.msg == pingNumber) {

                IPAddress remoteIp = udp_.remoteIP();
                uint16_t remotePort = udp_.remotePort();

                robotInitPacket pong = {pongNumber};
                
                for(int i = 0; i < 3; i++) {
                    udp_.beginPacket(remoteIp, remotePort);
                    udp_.write((uint8_t*)&pong, sizeof(pong));
                    udp_.endPacket();
                    delay(10); 
                }
                connected = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }

    Serial.println("Handshake received!");
    return true;
}

bool Communication::udpReceiveCmd(robotCmdPacket *cmd) {
    bool newPacket = false;
    int packetSize;
    
    while ((packetSize = udp_.parsePacket()) > 0) {
        if (packetSize == cmdPacketSize_) {
            udp_.read((uint8_t*) cmd, cmdPacketSize_);
            newPacket = true;
        } else {
            udp_.flush();
        }
    }

    return newPacket;
}

void Communication::udpSendState(robotStatePacket state) {

    udp_.beginPacket(udp_.remoteIP(), udp_.remotePort());
    udp_.write((uint8_t*)&state, statePacketSize_);
    udp_.endPacket();
}