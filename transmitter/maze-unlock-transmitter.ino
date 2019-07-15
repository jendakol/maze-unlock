#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "printf.h"
#include "maze.h"
#include "maze.cpp"

#define MU_CHANNEL_OFFSET 105
#define MU_PIN_CE 9
#define MU_PIN_CSN 8
#define MU_PIN_IDENTITY 7
#define MU_PIN_JOY_X A6
#define MU_PIN_JOY_Y A5
#define MU_PIN_JOY_BUTTON 3
#define MU_PIN_BUZZER 6
#define MU_PIN_LED_RED 5
#define MU_PIN_LED_GREEN 4
#define MU_PIN_BUTTON 10
#define MU_PINS_CHANNEL {A0, A1, A2}

#define MU_MOVE_NONE 0

#define BUZZER_ENABLED true

#define MORSE_UNIT 200

RF24 radio(MU_PIN_CE, MU_PIN_CSN);

const byte address[6] = "1tran";
const byte address2[6] = "2tran";

int missedPings = 0;
long nextPing = 0;

void blinkRed(int length) {
    digitalWrite(MU_PIN_LED_RED, HIGH);
    delay(length);
    digitalWrite(MU_PIN_LED_RED, LOW);
}

void blinkGreen(int length) {
    digitalWrite(MU_PIN_LED_GREEN, HIGH);
    delay(length);
    digitalWrite(MU_PIN_LED_GREEN, LOW);
}

void beep(int length) {
    if (BUZZER_ENABLED) digitalWrite(MU_PIN_BUZZER, HIGH);
    delay(length);
    if (BUZZER_ENABLED) digitalWrite(MU_PIN_BUZZER, LOW);
}

void beepAndBlinkRed(int length) {
    if (BUZZER_ENABLED) digitalWrite(MU_PIN_BUZZER, HIGH);
    digitalWrite(MU_PIN_LED_RED, HIGH);
    delay(length);
    if (BUZZER_ENABLED) digitalWrite(MU_PIN_BUZZER, LOW);
    digitalWrite(MU_PIN_LED_RED, LOW);
}

void beepAndBlinkGreen(int length) {
    if (BUZZER_ENABLED) digitalWrite(MU_PIN_BUZZER, HIGH);
    digitalWrite(MU_PIN_LED_GREEN, HIGH);
    delay(length);
    if (BUZZER_ENABLED) digitalWrite(MU_PIN_BUZZER, LOW);
    digitalWrite(MU_PIN_LED_GREEN, LOW);
}

void beepAndBlinkOrange(int length) {
    if (BUZZER_ENABLED) digitalWrite(MU_PIN_BUZZER, HIGH);
    digitalWrite(MU_PIN_LED_GREEN, HIGH);
    digitalWrite(MU_PIN_LED_RED, HIGH);
    delay(length);
    if (BUZZER_ENABLED) digitalWrite(MU_PIN_BUZZER, LOW);
    digitalWrite(MU_PIN_LED_GREEN, LOW);
    digitalWrite(MU_PIN_LED_RED, LOW);
}

int digitalReadPullup(int pin) { return (digitalRead(pin) + 1) % 2; }

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

void sendStartPacket() {
    char rawData[2] = "s";

    if (radio.write(&rawData, sizeof(rawData))) {
        if (radio.isAckPayloadAvailable()) {
            char ackPayload[3] = {0};
            radio.read(&ackPayload, sizeof(ackPayload));
            Serial.print("Start ACK payload: ");
            Serial.println(ackPayload);
        }

        beepAndBlinkGreen(15);
        delay(400);
    } else {
        Serial.println("Could not sent start packet!");
        beep(100);
        blinkRed(50);
        beep(100);
        blinkRed(50);
        beep(300);
        blinkRed(50);
    }
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

    pinMode(MU_PIN_BUTTON, INPUT_PULLUP);

    int identitySwitch = digitalReadPullup(MU_PIN_IDENTITY);
    Serial.println(digitalReadPullup(MU_PIN_IDENTITY) == HIGH ? "ClientId: 2" : "ClientId: 1");

    initRadio();

    radio.setAutoAck(true);
    radio.enableAckPayload();
    radio.enableDynamicPayloads();
    radio.setDataRate(RF24_250KBPS);
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_MAX);

    int channel = getChannelNumber(channelPins);
    radio.setChannel(channel);

    radio.openWritingPipe(identitySwitch == HIGH ? address : address2);
    radio.stopListening();

    radio.printDetails();
    Serial.println();

    sendStartPacket();
}

int readMoveDirection() {
    int valueX = map((int) fabs(analogRead(MU_PIN_JOY_X)), 0, 1023, 0, 255);

    if (valueX > 200) return MU_MOVE_RIGHT;
    if (valueX < 55) return MU_MOVE_LEFT;

    int valueY = map((int) fabs(analogRead(MU_PIN_JOY_Y)), 0, 1023, 0, 255);

    if (valueY > 200) return MU_MOVE_DOWN;
    if (valueY < 55) return MU_MOVE_UP;

    return MU_MOVE_NONE;
}

void sendProgress() {
    char rawData[2] = "i";

    if (radio.write(&rawData, sizeof(rawData))) {
        beepAndBlinkGreen(300);
    } else {
        Serial.println("Could not sent data!");
        beepAndBlinkRed(1000);
        delay(100);
        beepAndBlinkRed(1000);
        delay(100);
        beepAndBlinkRed(1000);
        delay(100);
    }
}


int phase = 0;
int inPhaseMoves = 0;

void wrongMove() {
    Serial.println("WRONG MOVE!!!");
    beep(300);
    delay(100);
    beep(300);
    delay(100);
    beep(300);
    delay(100);

    inPhaseMoves = 0;
    // TODO block the progress for a while
}

void checkMove(int direction) {
    int expectedDirection = mazeMoves[phase][0];
    int expectedMoves = mazeMoves[phase][1];

    inPhaseMoves++;

    if (direction != expectedDirection) {
        wrongMove();
        return;
    }

    if (inPhaseMoves > expectedMoves) {
        wrongMove();
        return;
    }

    // direction fits, there's still place for move

    beep(30);

    Serial.print("Phase ");
    Serial.print(phase);
    Serial.print(", move ");
    Serial.print(inPhaseMoves);
    Serial.print(", expected ");
    Serial.println(expectedMoves);
}

void endPhase() {
    int expectedMoves = mazeMoves[phase][1];

    if (inPhaseMoves == expectedMoves) {
        sendProgress();
        phase++;
        inPhaseMoves = 0;

        Serial.print("New phase: ");
        Serial.println(phase);
    } else {
        Serial.print("Phase ");
        Serial.print(phase);
        Serial.print(", move ");
        Serial.print(inPhaseMoves);
        Serial.print(" != expected ");
        Serial.println(expectedMoves);

        wrongMove();
    }
}

void beepDot() { beepAndBlinkOrange(MORSE_UNIT); }

void beepDash() { beepAndBlinkOrange(MORSE_UNIT * 3); }

void beepNumber(int dots1, int dashes, int dots2) {
    if (dots1 > 0)
        for (int i = 0; i < dots1; ++i) {
            Serial.print(".");
            beepDot();
            delay(MORSE_UNIT);
        }

    if (dashes > 0)
        for (int i = 0; i < dashes; ++i) {
            Serial.print("_");
            beepDash();
            delay(MORSE_UNIT);
        }

    if (dots2 > 0)
        for (int i = 0; i < dots2; ++i) {
            Serial.print(".");
            beepDot();
            delay(MORSE_UNIT);
        }

    Serial.println("|");
    delay(MORSE_UNIT);
    delay(MORSE_UNIT);
}

void beepNumber(int n) {
    if (n == 0) {
        beepNumber(0, 5, 0);
    } else if (n <= 5) {
        beepNumber(n, 5 - n, 0);
    } else {
        beepNumber(0, n - 5, 5 - (n - 5));
    }
}

void beepCurrentPhase() {
    String p = String(phase);

    beepNumber(p.charAt(0) - '0');

    if (p.length() > 1) {
        beepNumber(p.charAt(1) - '0');
    }
}

void loop() {
    if (digitalReadPullup(MU_PIN_BUTTON) == HIGH) {
        beepCurrentPhase();
        delay(1000);
    }

    int moveDirection = readMoveDirection();

    if (moveDirection == MU_MOVE_NONE) {
        if (digitalRead(MU_PIN_JOY_BUTTON) == LOW) {
            endPhase();
            delay(400);
        }
    } else {
        checkMove(moveDirection);
        delay(400);
    }

    if (millis() >= nextPing) {
        char rawData[10] = "p";
        char *data = *(&rawData) + 1;
        itoa(phase, data, 10);

        if (radio.write(&rawData, sizeof(rawData))) {
            blinkGreen(10);

            if (radio.isAckPayloadAvailable()) {
                char ackPayload[3];
                radio.read(&ackPayload, sizeof(ackPayload));
                Serial.print("Receiver reports level: ");
                phase = atoi(ackPayload);
                Serial.println(phase);
            }

            missedPings = 0;
        } else {
            missedPings++;
            Serial.print("Could not sent ping, missed ");
            Serial.println(missedPings);

            if (missedPings >= 2) {
                digitalWrite(MU_PIN_LED_RED, HIGH);
                beep(80);
                delay(50);
                beep(80);
                delay(50);
                beep(250);
                digitalWrite(MU_PIN_LED_RED, LOW);
            }
        }

        nextPing = millis() + random(950, 1050); // next in 950-1050ms
    }
}