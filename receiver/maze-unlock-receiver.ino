#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <Adafruit_NeoPixel.h>

#define MU_PIN_CE 9
#define MU_PIN_CSN 8
#define MU_PIN_LED 8

RF24 radio(MU_PIN_CE, MU_PIN_CSN);

const byte address[6] = "1tran";
const byte address2[6] = "2tran";

Adafruit_NeoPixel LED = Adafruit_NeoPixel(24, MU_PIN_LED, NEO_GRB + NEO_KHZ800);

int blue = 0, red = 0;

void setup() {
    Serial.begin(9600);
    delay(100);

    if (!radio.begin()) Serial.println("Could not initialize radio!!!"); else Serial.println("Radio initialized");

    radio.openReadingPipe(1, address);
    radio.openReadingPipe(2, address2);
    radio.startListening();
    LED.begin();
}

void loop() {
    byte clientId = 0;

    if (radio.available(&clientId)) {
        char text[3] = {0};
        radio.read(&text, sizeof(text));
        int level = atoi(text);

//        Serial.print("Sender: ");
//        Serial.println(clientId);

        if (clientId==1) red = level; else blue = level;

        for (int i = 0; i < 24; i++) {
            LED.setPixelColor(i, red, blue, 10);
        }

        LED.show();
    }
}
