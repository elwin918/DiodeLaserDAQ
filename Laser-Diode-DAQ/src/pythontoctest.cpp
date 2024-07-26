#include <Arduino.h>

const int maxSize = 10;  // Maximum number of elements to store
int values[maxSize];      // Array to store the values
int currentIndex = 0;     // Index to keep track of the current position

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        ;  // wait for serial port to connect
    }
}

void loop() {
    if (Serial.available() > 0) {
        int value = Serial.parseInt();  // Read an integer from serial
        if (currentIndex < maxSize) {
            values[currentIndex++] = value;  // Store the value in the array
        } else {
            Serial.println("Array is full");
        }
    }

    // For debugging: Print the array every 10 seconds
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime >= 10000) {
        lastPrintTime = millis();
        Serial.println("Current values in the array:");
        for (int i = 0; i < currentIndex; i++) {
            Serial.println(values[i]);
        }
    }
}
