#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-multiway-paths-covered"

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <Adafruit_NeoPixel.h>
#include <printf.h>

//přidat RST button

#define MU_CHANNEL_OFFSET 105
#define MU_PIN_CE 10
#define MU_PIN_CSN 2
#define MU_PIN_LEDS A2
#define MU_PIN_LED_RED A0
#define MU_PIN_LED_GREEN A1
#define MU_PIN_BUZZER 3
#define MU_PINS_CHANNEL {6, 5, 4}
#define MU_LEDS 24
#define MU_LED_ANIM_DELAY 40
#define MU_PHASES_MULT 4
#define MU_MAX_PHASES MU_LEDS * MU_PHASES_MULT
#define MU_COLOR_RED Adafruit_NeoPixel::Color(0, 255, 0)
#define MU_COLOR_BLUE Adafruit_NeoPixel::Color(0, 0, 255)
#define MU_COLOR_COMBINED Adafruit_NeoPixel::Color(0, 255, 255)
#define MU_COLOR_NONE Adafruit_NeoPixel::Color(0, 0, 0)

#define BUZZER_ENABLED true
#define LED_BRIGHTNESS 10 // TODO change when prod

RF24 radio(MU_PIN_CE, MU_PIN_CSN);

const byte address[6] = "1tran";
const byte address2[6] = "2tran";

Adafruit_NeoPixel LED = Adafruit_NeoPixel(MU_LEDS, MU_PIN_LEDS, NEO_GRB + NEO_KHZ800);

int phases[2] = {0, 0};

int currentPhase(int clientId) {
    return phases[clientId - 1];
}

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
    delay(10);
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

    initRadio();

    radio.setAutoAck(true);
    radio.enableAckPayload();
    radio.enableDynamicPayloads();
    radio.setDataRate(RF24_250KBPS);
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_MAX);

    int channel = getChannelNumber(channelPins);
    radio.setChannel(channel);

    radio.openReadingPipe(1, address);
    radio.openReadingPipe(2, address2);
    radio.startListening();
    LED.begin();
    radio.printDetails();
    Serial.println();

    LED.setBrightness(LED_BRIGHTNESS);
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

    beepAndBlinkGreen(15);
}

void drawBlinking(int ledIndex, int level, uint32_t color) {
    int drawPhase = (millis() % 2000) / 250;

    switch (drawPhase) {
        case 0: {
            // all levels on
            LED.setPixelColor(ledIndex, color);
        }
            break;
        case 1: {
            if (level == 3) {
                LED.setPixelColor(ledIndex, MU_COLOR_NONE);
            }
        }
            break;
        case 2: {
            switch (level) {
                case 2:
                    LED.setPixelColor(ledIndex, MU_COLOR_NONE);
                    break;
                case 3:
                    LED.setPixelColor(ledIndex, color);
                    break;
            }
        }
            break;
        case 3: {

            if (level == 3) {
                LED.setPixelColor(ledIndex, MU_COLOR_NONE);
            }
        }
            break;
        case 4: {
            switch (level) {
                case 1:
                    LED.setPixelColor(ledIndex, MU_COLOR_NONE);
                    break;
                case 2:
                case 3:
                    LED.setPixelColor(ledIndex, color);
                    break;
            }
        }
            break;
        case 5: {
            if (level == 3) {
                LED.setPixelColor(ledIndex, MU_COLOR_NONE);
            }
        }
            break;
        case 6: {
            switch (level) {
                case 2:
                    LED.setPixelColor(ledIndex, MU_COLOR_NONE);
                    break;
                case 3:
                    LED.setPixelColor(ledIndex, color);
                    break;
            }
        }
            break;
        case 7: {
            if (level == 3) {
                LED.setPixelColor(ledIndex, MU_COLOR_NONE);
            }
        }
            break;
    }
}

void drawProgress() {
    int common = min(phases[0], phases[1]);

    int commonFull = common / MU_PHASES_MULT;
    int commonRestLevel = common % MU_PHASES_MULT;

    int client1Full = phases[0] / MU_PHASES_MULT;
    int client1RestLevel = phases[0] % MU_PHASES_MULT;

    int client2Full = phases[1] / MU_PHASES_MULT;
    int client2RestLevel = phases[1] % MU_PHASES_MULT;


    // switch on common full
    for (int i = 0; i < commonFull; i++) {
        LED.setPixelColor(i, MU_COLOR_COMBINED);
    }

    // switch on client1 full
    for (int i = commonFull + (commonRestLevel > 0 ? 1 : 0); i < client1Full; i++) { // skip the first which if it's incomplete common
        LED.setPixelColor(i, MU_COLOR_RED);
    }

    // switch on client2 full
    for (int i = commonFull + (commonRestLevel > 0 ? 1 : 0); i < client2Full; i++) { // skip the first which if it's incomplete common
        LED.setPixelColor(i, MU_COLOR_BLUE);
    }

    if (client1Full == client2Full) {
        if (client1RestLevel > 0 && client2RestLevel > 0) {
            drawBlinking(commonFull, max(client1RestLevel, client2RestLevel), MU_COLOR_COMBINED);
        } else {
            if (client1RestLevel > 0) {
                drawBlinking(commonFull, client1RestLevel, MU_COLOR_RED);
            } else {
                if (client2RestLevel > 0) drawBlinking(commonFull, client2RestLevel, MU_COLOR_BLUE);
            }
        }
    } else {
        if (client1Full > client2Full) {
            if (client1RestLevel > 0) drawBlinking(client1Full, client1RestLevel, MU_COLOR_RED);
        } else {
            if (client2RestLevel > 0) drawBlinking(client2Full, client2RestLevel, MU_COLOR_BLUE);
        }

        if (commonRestLevel > 0) drawBlinking(commonFull, commonRestLevel, MU_COLOR_COMBINED);
    }

    LED.show();
}

int clientProgress(int clientId, int phase) {
    phases[clientId - 1] = phase;

    beep(15);

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

        if (reportedPhase >= MU_MAX_PHASES) finish(clientId);
    }
}

void loop() {
    byte clientId = 0;

    drawProgress();

    if (radio.available(&clientId)) {
        char rawData[10] = {0};
        radio.read(&rawData, sizeof(rawData));

        drawProgress();

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

                drawProgress();
            }
                break;
            case 'p': { // ping
                updateClientPhase(clientId, data);
                drawProgress();
                blinkGreen(2);
            }
                break;
            case 'i': { // increment
                int clientPhase = clientProgress(clientId, currentPhase(clientId) + 1);

                Serial.print("Client ");
                Serial.print(clientId);
                Serial.print(" new phase: ");
                Serial.println(clientPhase);

                if (clientPhase >= MU_MAX_PHASES) finish(clientId); else drawProgress();
            }
                break;

            default: {
                Serial.print("Not a valid command: ");
                Serial.println(rawData);
            }
        }
    }
}

#pragma clang diagnostic pop