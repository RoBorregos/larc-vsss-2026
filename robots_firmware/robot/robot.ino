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
#define motorA1 20
#define motorA2 22
#define motorAPWM 21
#define motorAENC 23

// define right motor pins
#define motorB1 24
#define motorB2 25
#define motorBPWM 26
#define motorBENC 27

// define left motor pins
#define motorC1 28
#define motorC2 29
#define motorCPWM 30
#define motorCENC 31

// define encoders resolution and sampling time
#define noTicks 350.0
#define dt 0.005

// Begin pulses counters
volatile unsigned long frontPulses = 0;
volatile unsigned long rightPulses = 0;
volatile unsigned long leftPulses = 0;

portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

// Update pulses
void IRAM_ATTR updateFrontPulses() {
  portENTER_CRITICAL_ISR(&spinlock);
  frontPulses++;
  portEXIT_CRITICAL_ISR(&spinlock);
}

void IRAM_ATTR updateRightPulses() {
  portENTER_CRITICAL_ISR(&spinlock);
  rightPulses++;
  portEXIT_CRITICAL_ISR(&spinlock);

}

void IRAM_ATTR updateLeftPulses() {
  portENTER_CRITICAL_ISR(&spinlock);
  leftPulses++;
  portEXIT_CRITICAL_ISR(&spinlock);
}

// Timer pointer
hw_timer_t *timer = NULL;

// Semaphore that allows control task to work
SemaphoreHandle_t executeControl;

// avoids issues when handling velSP
SemaphoreHandle_t mutex;

// Begins control
void IRAM_ATTR timerInterruption() {
  xSemaphoreGiveFromISR(executeControl, NULL);
}

// motors struct for three wheeled base
struct Motors {
  MotorController front {motorA1, motorA2, motorAPWM, dt};
  MotorController right {motorB1, motorB2, motorBPWM, dt};
  MotorController left {motorC1, motorC2, motorCPWM, dt};
};

// motors initialization
Motors motors;

// the pwm signal each motor will receive
int frontVel = 0;
int rightVel = 0;
int leftVel = 0;

// Task that manages control
void taskControl(void *parameter) {
  for (;;) {
    // check if the task can do the control
    if (xSemaphoreTake(executeControl, portMAX_DELAY) == pdTRUE) {

      readEncoders();

      // if the variable velSP is being written, do not read it
      if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        // compute the pwm signal of each motor
        frontVel = motors.front.PID();
        rightVel = motors.right.PID();
        leftVel = motors.left.PID();

        xSemaphoreGive(mutex);
      }
      drive(frontVel, rightVel, leftVel);
    }
  }
}

// task that receives the velocity of each wheel
void taskCommunication(void *parameter) {
  for (;;) {
    int expectedSize = 3 * sizeof(float);
    int packetSize = udp.parsePacket();
    if (packetSize == expectedSize) { // Expecting exactly 12 bytes (3 floats)
      byte buffer[expectedSize];
      udp.read(buffer, expectedSize);

      // if the variable velSP is being read, do not write on it
      if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        // Extract the two floats from the UDP packet
        memcpy(&motors.front.velSP, &buffer[0], 4);
        memcpy(&motors.right.velSP, &buffer[4], 4);
        memcpy(&motors.left.velSP, &buffer[8], 4);
        xSemaphoreGive(mutex);
      }

      Serial.print("Velocity set points: \nFront Motor: ");
      Serial.println(motors.front.velSP);
      Serial.print("Right Motor: ");
      Serial.println(motors.right.velSP);
      Serial.print("Left Motor: ");
      Serial.println(motors.left.velSP);

    } else if (packetSize > 0) {
      // Discard unexpected packet sizes
      Serial.print("Received unexpected UDP packet size: ");
      Serial.println(packetSize);
      udp.flush(); // Clear the packet
    }
  }
}

void setup() {
  Serial.begin(115200);

    // Set timer
  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &timerInterruption);

  // semaphore that executes contorl every 5ms
  executeControl = xSemaphoreCreateBinary();

  mutex = xSemaphoreCreateMutex();

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

  timerAlarm(timer, dt * 1000000, true, 0);

  // tasks definition
  xTaskCreatePinnedToCore(taskCommunication,
                          "Communication",
                          4096,
                          NULL,
                          1,
                          NULL,
                          0
  );

  xTaskCreatePinnedToCore(taskControl,
                          "control",
                          4096,
                          NULL,
                          5,
                          NULL,
                          1
  );
}

void drive(int frontSpeed, int rightSpeed, int leftSpeed) {
  motors.front.move(frontSpeed);
  motors.right.move(rightSpeed);
  motors.left.move(leftSpeed);
}

void readEncoders() {

  portENTER_CRITICAL(&spinlock);
  int fPulses = frontPulses; frontPulses = 0;
  int rPulses = rightPulses; rightPulses = 0;
  int lPulses = leftPulses; leftPulses = 0;
  portEXIT_CRITICAL(&spinlock);

  motors.front.velReal = fPulses * 60 / (noTicks * dt);
  motors.right.velReal = rPulses * 60 / (noTicks * dt);
  motors.left.velReal = lPulses * 60 / (noTicks * dt);

  motors.front.velReal *= motors.front.PWMReal < 0 ? -1 : 1;
  motors.right.velReal *= motors.right.PWMReal < 0 ? -1 : 1;
  motors.left.velReal *= motors.left.PWMReal < 0 ? -1 : 1;
}


void loop() {
  
}
