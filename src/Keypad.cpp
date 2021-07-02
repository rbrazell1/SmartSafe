//
// Created by Russell on 7/1/2021.
//

#ifndef SMARTSAFE_KEYPAD_H
#define SMARTSAFE_KEYPAD_H


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
const int SERVO_VAULT_DOOR_PIN = 22;
const int UNLOCKED = 0;
const int LOCKED = 180;
const int STEPS_PER_REV = 2048;
const int STEPPER_ONE = 14;
const int STEPPER_TWO = 15;
const int STEPPER_THREE = 16;
const int STEPPER_FOUR = 17;

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
Stepper CandyStepper(stepsPerRevolution, STEPPER_ONE, STEPPER_TWO, STEPPER_THREE, STEPPER_FOUR);

void setUpKeypad() {
    display.println("Serial Com established...");
    EthernetStatus = Ethernet.begin(mac);
    if (!EthernetStatus) {
        display.printf("failed to configure Ethernet using DHCP \n");
        //no point in continuing
    }
    trapDoorServo.attach(SERVO_TRAP_DOOR_PIN);
    vaultDoorServo.attach(SERVO_VAULT_DOOR_PIN);
    codeIndex = 0;
    digitsCorrect = 0;
    pinMode(10, OUTPUT);
    pinMode(4, OUTPUT);
    digitalWrite(10, HIGH);
    digitalWrite(4, HIGH);
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
    display.printf("Enter your code\n");
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
        openDoor();
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
   Diplay.pickCandy();
   Serial.readIn("");
}

void nextOutlet() {
    if (outlet > 3) {
        outlet = 0;
    }
    wemo.turnOn(outlet);
    outlet++;
}


#endif //SMARTSAFE_KEYPAD_H
