#include <WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include "MotorController.h"

// Replace with your server/router ,etc 
const char* ssid = "iPhone von iker";
const char* password = "iker1595";

// UDP Server
WiFiUDP udp;
const int localPort = 8081;

struct __attribute__((packed)) Packet {
  float w;
  float x;
  float y;
  float z;
};

// TODO: set pins to actual values

// define front motor pins
#define motorA1 26
#define motorA2 27
#define motorAPWM 14
#define motorAENC1 13
#define motorAENC2 25

// define right motor pins
#define motorB1 6
#define motorB2 7
#define motorBPWM 8
#define motorBENC1 9
#define motorBENC2 GPIO_NUM_10

// define left motor pins
#define motorC1 11
#define motorC2 12
#define motorCPWM 13
#define motorCENC1 14
#define motorCENC2 GPIO_NUM_15

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
  if (gpio_get_level(motorAENC2)){
    frontPulses++;
  } else {
    frontPulses--;
  }
  portEXIT_CRITICAL_ISR(&spinlock);
}

void IRAM_ATTR updateRightPulses() {
  portENTER_CRITICAL_ISR(&spinlock);
  if (gpio_get_level(motorBENC2)) {
    rightPulses++;
  } else {
    rightPulses--;
  }
  portEXIT_CRITICAL_ISR(&spinlock);
}

void IRAM_ATTR updateLeftPulses() {
  portENTER_CRITICAL_ISR(&spinlock);
  if (gpio_get_level(motorCENC2)) {
    leftPulses++;
  } else {
    leftPulses--;
  }
  portEXIT_CRITICAL_ISR(&spinlock);
}

// Timer pointer
hw_timer_t *timerControl = NULL;
hw_timer_t *timerGyro = NULL;

// Semaphore that allows control task to work
SemaphoreHandle_t executeControl;
SemaphoreHandle_t readGyro;

// avoids issues when handling velSP
SemaphoreHandle_t mutex;

// Begins control
void IRAM_ATTR timerInterruption() {
  xSemaphoreGiveFromISR(executeControl, NULL);
}

void IRAM_ATTR timerInterruption2() {
  xSemaphoreGiveFromISR(readGyro, NULL);
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

// Gyroscope definition
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);

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

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// task that receives the velocity of each wheel
void taskCommunication(void *parameter) {
  for (;;) {
    int expectedSize = 3 * sizeof(float);
    int packetSize = udp.parsePacket();
    if (packetSize == expectedSize) { // Expecting exactly 12 bytes (3 floats)
      Serial.println("Received value with expected size of 12 bytes");
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

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
void taskReadGyro(void *parameter) {
  float w = 0, x = 0, y = 0, z = 0;

  // unsigned long previous_send = millis();
  for (;;) {
  //   if ((millis() - previous_send > 1000) && xSemaphoreTake(readGyro, portMAX_DELAY) == pdTRUE) {

  //     // Read gyroscope
  //     imu::Quaternion quat = bno.getQuat();

  //     Packet data;

  //     data.w = quat.w();
  //     data.x = quat.x();
  //     data.y = quat.y();
  //     data.z = quat.z();

  //     udp.beginPacket(udp.remoteIP(), udp.remotePort());
  //     udp.write((uint8_t*)&data, sizeof(data));
  //     udp.endPacket();

  //     previous_send = millis();
  //   }
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println("SERIAL STARTED");

  // // Set timer
  timerControl = timerBegin(1000000);
  timerAttachInterrupt(timerControl, &timerInterruption);

  // // semaphore that executes contorl every 5ms
  executeControl = xSemaphoreCreateBinary();

  timerGyro = timerBegin(10000000);
  timerAttachInterrupt(timerGyro, &timerInterruption2);

  readGyro = xSemaphoreCreateBinary();

  mutex = xSemaphoreCreateMutex();

  //Set motors
  motors.front.setMotor();
  // motors.right.setMotor();
  // motors.left.setMotor();
  
  // Set encoders
  pinMode(motorAENC1, INPUT_PULLUP);
  pinMode(motorAENC2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(motorAENC1), updateFrontPulses, RISING);

  // pinMode(motorBENC1, INPUT_PULLUP);
  // pinMode(motorBENC2, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(motorBENC1), updateRightPulses, RISING);

  // pinMode(motorCENC1, INPUT_PULLUP);
  // pinMode(motorCENC2, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(motorCENC1), updateLeftPulses, RISING);

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


//   /* Initialise the sensor */
//   if (!bno.begin())
//   {
//     /* There was a problem detecting the BNO055 ... check your connections */
//     Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
//     while (1);
//   }

  timerAlarm(timerControl, dt * 1000000, true, 0);
  timerAlarm(timerGyro, 10000, true, 0);

  // tasks definition
  xTaskCreatePinnedToCore(taskCommunication,
                          "Communication",
                          4096,
                          NULL,
                          1,
                          NULL,
                          tskNO_AFFINITY
  );

  xTaskCreatePinnedToCore(taskControl,
                          "control",
                          4096,
                          NULL,
                          5,
                          NULL,
                          1
  );

  xTaskCreatePinnedToCore(taskReadGyro,
                          "Telemetry",
                          4096,
                          NULL,
                          5,
                          NULL,
                          1
  );
}

void drive(int frontSpeed, int rightSpeed, int leftSpeed) {
  motors.front.move(frontSpeed);
  // motors.right.move(rightSpeed);
  // motors.left.move(leftSpeed);
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
}


void loop() {
  
}
