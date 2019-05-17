#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <Adafruit_NeoPixel.h>

//create an RF24 object
RF24 radio(9, 8);  // CE, CSN

//address through which two modules communicate.
const byte address[6] = "00001";

Adafruit_NeoPixel LED = Adafruit_NeoPixel(24, 5, NEO_GRB + NEO_KHZ800);
int zelena, cervena, modra;

void setup() {
    Serial.begin(9600);

//    int val = 0 +  (((digitalRead(7) + 1) % 2) << 0);
//    Serial.println(val, BIN);
//    itoa(val, address);

    radio.begin();

    radio.openReadingPipe(0, address);
    radio.startListening();
    LED.begin();
}

void loop() {
    if (radio.available()) {
        char text[3] = {0};
        radio.read(&text, sizeof(text));
        int level = atoi(text);

        int ledIndex = map(level, 0, 255, 0, 24);

        Serial.print("LED index: ");
        Serial.println(ledIndex);

        for (int i = 0; i < ledIndex; i++) {
            LED.setPixelColor(i, 10, 200, 200);
            LED.show();
        }

        for (int i = ledIndex; i < 24; i++) {
            LED.setPixelColor(i, 0, 0, 0);
            LED.show();
        }
    }
}
