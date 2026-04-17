#include "Communication.hpp"
#include "Motor.hpp"

#include "Constants.hpp"
#include "Config.hpp"

Communication udpClientServer(
    WIFI_SSID,
    WIFI_PASSWORD,
    LOCAL_PORT
);

Motor motorLeft(
    0,
    inA1,
    inA2,
    PWMA,
    ENCA
);

Motor motorBack(
    1,
    inB1,
    inB2,
    PWMB,
    ENCB
);

Motor motorRight(
    2,
    inC1,
    inC2,
    PWMC,
    ENCC
);

void setup() {
    Serial.begin(9600);

    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);

    udpClientServer.configComms();
    udpClientServer.commsInit();

    motorLeft.begin();
    motorRight.begin();
    motorBack.begin();

    motorLeft.attach_kterms(10, 360, 0);
    motorRight.attach_kterms(10, 250, 0);
    motorBack.attach_kterms(10, 210, 0);
}

TickType_t lastWakeTime = xTaskGetTickCount();


unsigned long previous_led_toggle = millis();
unsigned long previous_udp_update = millis();
unsigned long previous_motor_update = millis();
unsigned long previous_debug_update = millis();
bool led_state = true;

float left_rps = 0;
float right_rps = 0;
float back_rps = 0;
bool stop = false;
robotCmdPacket cmd;


void loop() {
    motorLeft.tick();
    motorRight.tick();
    motorBack.tick();

    if ((millis() - previous_led_toggle) >= 1000) {
        digitalWrite(15, led_state);

        led_state = !led_state;
        previous_led_toggle = millis();
    }

    if ((millis() - previous_udp_update) >= 25) {
        udpClientServer.udpReceiveCmd(&cmd);

        left_rps = cmd.rpm_left / 60;
        right_rps = cmd.rpm_right / 60;
        back_rps = cmd.rpm_back / 60;
        stop = cmd.stop;

        robotStatePacket state;
        state.rpm_left  = motorLeft.enc.get_rps() * 60;
        state.rpm_right = motorRight.enc.get_rps() * 60;
        state.rpm_back  = motorBack.enc.get_rps() * 60;
        state.yaw = 0;

        udpClientServer.udpSendState(state);

        previous_udp_update = millis();
    }

    if ((millis() - previous_motor_update) >= 25) {
        if (stop) {
            motorLeft.brake();
            motorRight.brake();
            motorBack.brake();
        } else {
            motorLeft.move(left_rps);
            motorRight.move(right_rps);
            motorBack.move(back_rps);
        }

        previous_motor_update = millis();
    }

    if ((millis() - previous_debug_update) >= 100) {
        motorLeft.print_debug();
        motorRight.print_debug();
        motorBack.print_debug();

        previous_debug_update = millis();
    }
}