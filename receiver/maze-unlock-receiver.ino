#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <Adafruit_NeoPixel.h>
#include <printf.h>

#define MU_CHANNEL_OFFSET 105
#define MU_PIN_CE 9
#define MU_PIN_CSN 8
#define MU_PIN_LED 5
#define MU_PIN_BUZZER 6
#define MU_PINS_CHANNEL {A0, A1, A2}
#define MU_LEDS 24
#define MU_LED_ANIM_DELAY 40

RF24 radio(MU_PIN_CE, MU_PIN_CSN);

const byte address[6] = "1tran";
const byte address2[6] = "2tran";

Adafruit_NeoPixel LED = Adafruit_NeoPixel(MU_LEDS, MU_PIN_LED, NEO_GRB + NEO_KHZ800);

int phases[2] = {0, 0};

int currentPhase(int clientId) {
    return phases[clientId-1];
}

void beep(int length) {
    digitalWrite(MU_PIN_BUZZER, HIGH);
    delay(length);
    digitalWrite(MU_PIN_BUZZER, LOW);
    delay(10);
}

int digitalReadPullup(int pin) {
    return (digitalRead(pin) + 1) % 2;
}

int getChannelNumber(const int *channelPins) {
//    int channel = 0;
//    if (digitalReadPullup(channelPins[0])) channel = bitSet(channel, 1);
//    if (digitalReadPullup(channelPins[1])) channel = bitSet(channel, 2);
//    if (digitalReadPullup(channelPins[2])) channel = bitSet(channel, 3);
//    return MU_CHANNEL_OFFSET + channel;

    // TODO fix after wiring
    return 109;
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

    initRadio();

    radio.setAutoAck(true);
    radio.enableAckPayload();
    radio.enableDynamicPayloads();
    radio.setDataRate(RF24_250KBPS);
    radio.setRetries(10, 15);
    radio.setPALevel(RF24_PA_MAX);

    int channel = getChannelNumber(channelPins);
    radio.setChannel(channel);

    radio.openReadingPipe(1, address);
    radio.openReadingPipe(2, address2);
    radio.startListening();
    LED.begin();
    radio.printDetails();
    Serial.println();

    LED.setBrightness(20);
    LED.show();

    for (int i = 0; i < MU_LEDS; i++) {
        delay(MU_LED_ANIM_DELAY);
        LED.setPixelColor(i, 255, 255, 255);
        LED.show();
    }

    delay(MU_LED_ANIM_DELAY * 2);

    for (int i = 0; i < MU_LEDS; i++) {
        LED.setPixelColor(i, 0, 0, 0);
    }
    LED.show();
}

int clientProgress(int clientId, int phase) {
    phases[clientId - 1] = phase;

    int common = min(MU_LEDS, min(phases[0], phases[1]));

    for (int i = 0; i < common; i++) {
        LED.setPixelColor(i, 0, 255, 255);
    }

    for (int i = common; i < phases[0]; i++) {
        LED.setPixelColor(i, 0, 255, 0);
    }

    for (int i = common; i < phases[1]; i++) {
        LED.setPixelColor(i, 0, 0, 255);
    }

    LED.show();
    beep(500);

    return phases[clientId - 1];
}


void finish(int clientId) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (true) {
        // on:
        for (int i = 0; i < MU_LEDS; i++) {
            switch (clientId) {
                case 1:
                    LED.setPixelColor(i, 0, 255, 0);
                    break;
                case 2:
                    LED.setPixelColor(i, 0, 0, 255);
                    break;
            }
        }
        LED.show();
        digitalWrite(MU_PIN_BUZZER, HIGH);
        delay(500);

        // off:
        for (int i = 0; i < MU_LEDS; i++) {
            LED.setPixelColor(i, 0, 0, 0);
        }
        LED.show();
        digitalWrite(MU_PIN_BUZZER, LOW);
        delay(500);
    }
#pragma clang diagnostic pop
}

void updateClientPhase(int clientId, char *data) {
    int reportedPhase = atoi(data);

    if (reportedPhase != currentPhase(clientId) && reportedPhase != 0) {
        Serial.print("Client ");
        Serial.print(clientId);
        Serial.print(" reports phase ");
        Serial.println(reportedPhase);

        clientProgress(clientId, reportedPhase);
    }
}

void loop() {
    byte clientId = 0;

    if (radio.available(&clientId)) {
        char rawData[10] = {0};
        radio.read(&rawData, sizeof(rawData));

        char *data = *(&rawData) + 1;

        switch (rawData[0]) {
            case 's': { // start
                Serial.print("Start packet from client ");
                Serial.println(clientId);

                int clientPhase = phases[clientId - 1];

                char ackPayload[3];
                itoa(clientPhase, ackPayload, 10);
                Serial.print("Sending back client ");
                Serial.print(clientId);
                Serial.print(" phase: ");
                Serial.println(ackPayload);
                radio.writeAckPayload(clientId, ackPayload, sizeof(ackPayload));
            }
                break;
            case 'p': { // ping
                updateClientPhase(clientId, data);
            }
                break;
            case 'i': { // increment
                int clientPhase = clientProgress(clientId, currentPhase(clientId) + 1);

                Serial.print("Client ");
                Serial.print(clientId);
                Serial.print(" new phase: ");
                Serial.println(clientPhase);

                if (clientPhase >= MU_LEDS) finish(clientId);
            }
                break;

            default: {
                Serial.print("Not a valid command: ");
                Serial.println(rawData);
            }
        }
    }
}
