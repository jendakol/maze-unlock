#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

//create an RF24 object
RF24 radio(9, 8);  // CE, CSN

//address through which two modules communicate.
const byte address[6] = "00001";

void setup() {
    Serial.begin(9600);

    pinMode(6, OUTPUT);
    pinMode(7, INPUT_PULLUP);

//    int val = 0 +  (((digitalRead(7) + 1) % 2) << 0);
//    Serial.println(val, BIN);
//    itoa(val, address);

    radio.begin();

    radio.openWritingPipe(address);
    radio.stopListening();
}

void loop() {
    int value = map((int) fabs(analogRead(A6)), 0, 1023, 0, 255);
    Serial.print("Level: ");
    Serial.println(value);

    if (digitalRead(3) == LOW) {
        digitalWrite(6, HIGH);
    } else digitalWrite(6, LOW);

    if (digitalRead(7) == LOW) {
        digitalWrite(5, HIGH);
    } else {
        digitalWrite(5, LOW);
    }

    char text[4];
    itoa(value, text, 10);
    radio.write(&text, sizeof(text));
}