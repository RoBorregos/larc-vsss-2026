#include <WiFi.h>
#include <WiFiUdp.h>
#include "MotorController.h"

// Replace with your server/router ,etc 
const char* ssid = "vsss_r";
const char* password = "vsss1234";

// UDP Server
WiFiUDP udp;
const int localPort = 8081;


// TODO: set pins to actual values

// define front motor pins
#define motorA1 1
#define motorA2 2
#define motorAPWM 3
#define motorAENC 4

// define right motor pins
#define motorB1 5
#define motorB2 6
#define motorBPWM 7
#define motorBENC 8

// define left motor pins
#define motorC1 9
#define motorC2 10
#define motorCPWM 11
#define motorCENC 12

// Begin pulses counters
volatile unsigned long frontPulses = 0;
volatile unsigned long rightPulses = 0;
volatile unsigned long leftPulses = 0;

// Update pulses
void IRAM_ATTR updateFrontPulses() {
  frontPulses += 1;
}

void IRAM_ATTR updateRightPulses() {
  rightPulses += 1;
}

void IRAM_ATTR updateLeftPulses() {
  leftPulses += 1;
}

// Timer pointer
hw_timer_t *timer = NULL;

// Turns true at sampling time
volatile bool executeControl = false;
void IRAM_ATTR timerInterruption() {
  executeControl = true;
}

struct Motors {
  MotorController front {motorA1, motorA2, motorAPWM};
  MotorController right {motorB1, motorB2, motorBPWM};;
  MotorController left {motorC1, motorC2, motorCPWM};;
};

Motors motors;

void setup() {
  Serial.begin(115200);
  //Set motors
  motors.front.setMotor();
  motors.right.setMotor();
  motors.left.setMotor();
  
  // Set encoders
  pinMode(motorAENC, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(motorAENC), updateFrontPulses, RISING);

  pinMode(motorBENC, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(motorBENC), updateRightPulses, RISING);

  pinMode(motorCENC, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(motorCENC), updateLeftPulses, RISING);

  // Set timer
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &timerInterruption);

  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start UDP server
  udp.begin(localPort);
  Serial.print("UDP server started on port ");
  Serial.println(localPort);

  timerAlarm(timer, 5000, true, 0);
}

void drive(int frontSpeed, int rightSpeed, int leftSpeed) {
  motors.front.move(frontSpeed);
  motors.right.move(rightSpeed);
  motors.left.move(leftSpeed);
}

void udp_receive_RPM() {
  int expectedSize = 3 * sizeof(float);
  int packetSize = udp.parsePacket();
  if (packetSize == expectedSize) { // Expecting exactly 12 bytes (3 floats)
    byte buffer[expectedSize];
    udp.read(buffer, expectedSize);

    // Extract the two floats from the UDP packet
    memcpy(&motors.front.velSP, &buffer[0], 4);
    memcpy(&motors.right.velSP, &buffer[4], 4);
    memcpy(&motors.left.velSP, &buffer[8], 4);

    Serial.print("Velocity set points: \nFront Motor: ")

  } else if (packetSize > 0) {
    // Discard unexpected packet sizes
    Serial.print("Received unexpected UDP packet size: ");
    Serial.println(packetSize);
    udp.flush(); // Clear the packet
  }
}

void loop() {

}