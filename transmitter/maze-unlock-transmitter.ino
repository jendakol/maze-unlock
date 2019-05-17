#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

#define MU_PIN_CE 9
#define MU_PIN_CSN 8
#define MU_PIN_IDENTITY 7
#define MU_PIN_JOY A6

RF24 radio(MU_PIN_CE, MU_PIN_CSN);

const byte address[6] = "1tran";
const byte address2[6] = "2tran";

void setup() {
    Serial.begin(9600);
    delay(100);

    pinMode(6, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(MU_PIN_IDENTITY, INPUT_PULLUP);

    int val = 0 + (((digitalRead(MU_PIN_IDENTITY) + 1) % 2) << 0); // NOLINT(hicpp-signed-bitwise)

    if (!radio.begin()) Serial.println("Could not initialize radio!!!"); else Serial.println("Radio initialized");

    radio.openWritingPipe(val == HIGH ? address : address2);
    radio.stopListening();
}

void loop() {
    int value = map((int) fabs(analogRead(MU_PIN_JOY)), 0, 1023, 0, 255);

    if (digitalRead(3) == LOW) {
        digitalWrite(6, HIGH);
    } else digitalWrite(6, LOW);

    if (digitalRead(MU_PIN_IDENTITY) == LOW) {
        digitalWrite(5, HIGH);
    } else {
        digitalWrite(5, LOW);
    }

    char text[4];
    itoa(value, text, 10);
    radio.write(&text, sizeof(text));

    delay(15);
}