//
// Created by Russell on 7/1/2021.
//

#ifndef SMARTSAFE_KEYPAD_H
#define SMARTSAFE_KEYPAD_H

/*
   Project: Servo lock
   Description: Code to move a unlock a bolt lock with key pad and servo
   Aurthor: Russell Brazell
   Date: 6-22-2021
*/

#include <math.h>
#include <PWMServo.h>
#include <Keypad.cpp>
#include <IOTTimer.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Mac.h>
#include <Wemo.h>
#include <Stepper.h>
#include "Display.cpp"

const int SERVO_TRAP_DOOR_PIN = 23;
const int SERVO_VAULT_DOOR_PIN = 23;
const int UNLOCKED = 0;
const int LOCKED = 180;
const int stepsPerRevolution = 2048;

const byte ROWS = 4;
const byte COLS = 4;

int codeIndex;
int rotation;
int digitsCorrect;
int stepCount;

static int outlet;

char customKey;

bool status;
bool isLocked;
bool lock;

char defaultCode[] = {'1', '1', '1', '1'};
char enteredCode[4];
byte rowPins[ROWS] = {9, 8, 7, 6}; // keypad leads 8 , 7 , 6 , 5
byte colPins[COLS] = {5, 4, 3, 2}; // keypad leads 4 , 3 , 2 , 1

char hexaKeys[ROWS][COLS] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
};

PWMServo trapDoorServo;
PWMServo vaultDoorServo;
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
EthernetClient client;
IOTTimer timer;
Wemo wemo;
Stepper CandyStepper(stepsPerRevolution, 8, 9, 10, 11);

void setup() {
    Serial.begin(9600);
    Serial.printf("Push a button on the key pad\n");
    basicServo.attach(SERVO_PIN);
    codeIndex = 0;
    digitsCorrect = 0;
    pinMode(PIN_GLED, OUTPUT);
    pinMode(PIN_RLED, OUTPUT);
    pinMode(10, OUTPUT);
    pinMode(4, OUTPUT);
    digitalWrite(10, HIGH);
    digitalWrite(4, HIGH);
    redButton.attachClick(turnOffDelayed);
    greenButton.attachClick(nextOutlet);
    Serial.begin(9600);
    while (!Serial);
    Serial.println("Serial Com established...");
    status = Ethernet.begin(mac);
    if (!status) {
        Serial.printf("failed to configure Ethernet using DHCP \n");
        //no point in continuing
        while (1);
    }
    //print your local IP address
    Serial.print("My IP address:");
    for (byte thisByte = 0; thisByte < 4; thisByte++) {
        Serial.print(Ethernet.localIP()[thisByte], DEC);
        if (thisByte < 3) Serial.print(".");
    }
    Serial.println();
    outlet = 0;
}

void loop() {
    customKey = customKeypad.getKey();
    if (customKey) {
        enterCode();
        if (codeIndex == 4) {
            checkCode();
        }
    }
}

void enterCode() {
    enteredCode[codeIndex] = customKey;
    codeIndex < 4 ? codeIndex++ : codeIndex = 3;
}

void checkCode() {
    Serial.printf("You entered: %c, %c, %c, %c\n",
                  enteredCode[0], enteredCode[1], enteredCode[2], enteredCode[3]);
    for (int i = 0; i < codeIndex; i++) {
        isLocked = enteredCode[i] == defaultCode[i];
        isLocked == true
        ? digitsCorrect++
        : digitsCorrect = 0;
    }

    codeIndex = 0;
    if (digitsCorrect >= 4) {
        moveBolt();
    }
    digitsCorrect = 0;
}

void openDoor() {
    if (lock == true) {
        locking();
    } else {
        unlocking();
    }
    lock = !lock;
    digitsCorrect = 0;
}

void locking() {
    trapDoorServo.write(LOCKED);
    vaultDoorServo.write(LOCKED);
}

void unlocking() {
   vaultDoorServo.write(UNLOCKED);
   pickCandy();
}

void nextOutlet() {
    if (outlet > 3) {
        outlet = 0;
    }
    wemo.turnOn(outlet);
    outlet++;
}


#endif //SMARTSAFE_KEYPAD_H