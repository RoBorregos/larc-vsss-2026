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

// PWM Properties
const int PWM_FREQUENCY = 5000; // 5 kHz
const int PWM_RESOLUTION = 8; // 8-bit resolution (0-255)
const int MAX_PWM_VALUE = 255;

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

void setup() {
  Serial.begin(115200);
  //Set motors
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);

  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);

  pinMode(motorC1, OUPUT);
  pinMode(motorC2, OUTPUT);

  // Set motors PWM
  ledcAttach(motorAPWM, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttach(motorBPWM, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttach(motorCPWM, PWM_FREQUENCY, PWM_RESOLUTION);
  
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
  timerAlarm(timer, 5000, true, 0);

}

void loop() {

}