#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "printf.h"

#define MU_CHANNEL_OFFSET 105
#define MU_PIN_CE 9
#define MU_PIN_CSN 8
#define MU_PIN_IDENTITY 7
#define MU_PIN_JOY_X A6
#define MU_PIN_BUZZER 6
#define MU_PIN_LED_RED 5
#define MU_PIN_LED_GREEN 4
#define MU_PINS_CHANNEL {A0, A1, A2}

RF24 radio(MU_PIN_CE, MU_PIN_CSN);

const byte address[6] = "1tran";
const byte address2[6] = "2tran";

void blinkRed(int length) {
    digitalWrite(MU_PIN_LED_RED, HIGH);
    delay(length);
    digitalWrite(MU_PIN_LED_RED, LOW);

    delay(10);
}

void blinkGreen(int length) {
    digitalWrite(MU_PIN_LED_GREEN, HIGH);
    delay(length);
    digitalWrite(MU_PIN_LED_GREEN, LOW);

    delay(10);
}

void beep(int length) {
//    digitalWrite(MU_PIN_BUZZER, HIGH);
//    delay(length);
//    digitalWrite(MU_PIN_BUZZER, LOW);
//    delay(10);
}

int digitalReadPullup(int pin) {
    return (digitalRead(pin) + 1) % 2;
}

int getChannelNumber(const int *channelPins) {
    int channel = 0;
    if (digitalReadPullup(channelPins[0])) channel = bitSet(channel, 1);
    if (digitalReadPullup(channelPins[1])) channel = bitSet(channel, 2);
    if (digitalReadPullup(channelPins[2])) channel = bitSet(channel, 3);
    return MU_CHANNEL_OFFSET + channel;
}

void initRadio() {
    if (!radio.begin()) {
        Serial.println("Could not initialize radio!!!");
        for (int i = 0; i < 5; i++) {
            beep(1000);
            delay(200);
        }
    } else Serial.println("Radio initialized");
}

void setup() {
    Serial.begin(9600);
    Serial.println("\n---\nStarting...\n");
    printf_begin();
    delay(100);

    int channelPins[] = MU_PINS_CHANNEL;

    pinMode(channelPins[0], INPUT_PULLUP);
    pinMode(channelPins[1], INPUT_PULLUP);
    pinMode(channelPins[2], INPUT_PULLUP);

    pinMode(MU_PIN_BUZZER, OUTPUT);
    pinMode(MU_PIN_LED_RED, OUTPUT);
    pinMode(MU_PIN_LED_GREEN, OUTPUT);
    pinMode(MU_PIN_IDENTITY, INPUT_PULLUP);

    int identitySwitch = digitalReadPullup(MU_PIN_IDENTITY) << 0;

    Serial.println(identitySwitch == HIGH ? "ClientId: 2" : "ClientId: 1");

    initRadio();

    radio.setAutoAck(true);
    radio.enableAckPayload();
    radio.enableDynamicPayloads();
    radio.setDataRate(RF24_250KBPS);
    radio.setRetries(10, 15);
    radio.setPALevel(RF24_PA_MAX);

    int channel = getChannelNumber(channelPins);
    radio.setChannel(channel);

    radio.openWritingPipe(identitySwitch == HIGH ? address : address2);
    radio.stopListening();

    radio.printDetails();
    Serial.println();
}

void loop() {
//    if (digitalRead(3) == LOW) {
//        digitalWrite(6, HIGH);
//    } else digitalWrite(6, LOW);

    int value = map((int) fabs(analogRead(MU_PIN_JOY_X)), 0, 1023, 0, 255);

    if (value > 200) {
        char rawData[2] = "p";

        if (radio.write(&rawData, sizeof(rawData))) {
            Serial.print("Sent: ");
            Serial.println(rawData);

//            if (radio.isAckPayloadAvailable()) {
//                char ackPayload[3];
//                radio.read(&ackPayload, sizeof(ackPayload));
//                Serial.print("ACK payload: ");
//                Serial.println(ackPayload);
//            }

            beep(15);
            delay(400);
        } else {
            Serial.println("Could not sent data!");
            beep(100);
            blinkRed(50);
//            beep(100);
//            beep(300);
        }
    }

    if ((millis() / 100) % 10 == 0) {
        char rawData[2] = "i";
        if (radio.write(&rawData, sizeof(rawData))) {
            blinkGreen(30);
        } else {
            Serial.println("Could not sent ping!");
            beep(100);
            blinkRed(50);
            beep(100);
            blinkRed(50);
            beep(300);
            blinkRed(50);
        }

        delay(100);
    }

}