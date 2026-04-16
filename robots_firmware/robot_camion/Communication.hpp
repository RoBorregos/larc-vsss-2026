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
  Communication(const char *ssid, const char *password, int local_port)
  : ssid_(ssid), password_(password), local_port_(local_port) {}

  wl_status_t configComms() {
    // Connect to WiFi
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid_);
    WiFi.begin(ssid_, password_);

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Start UDP server
    udp_.begin(local_port_);
    Serial.print("UDP server started on port ");
    Serial.println(local_port_);

    WiFi.setSleep(false);

    return WiFi.status();
  }

  bool commsInit() {
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
            vTaskDelay(pdMS_TO_TICKS(10)); 
          }
          
          connected = true;
        }
      }
          vTaskDelay(pdMS_TO_TICKS(100)); 
    }

    Serial.println("Handshake received!");
    return true;
  }

  bool udpReceiveCmd(robotCmdPacket *cmd) {
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

  void udpSendState(robotStatePacket state) {

    udp_.beginPacket(udp_.remoteIP(), udp_.remotePort());
    udp_.write((uint8_t*)&state, statePacketSize_);
    udp_.endPacket();
  }
};

#endif
