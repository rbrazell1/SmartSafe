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
#include <Hue.h>
#include <TM1637.h>
#include "Display.cpp"

const int SERVO_TRAP_DOOR_PIN = 23;
const int SERVO_VAULT_DOOR_PIN = 22;
const int IC_CLK_PIN = 19;
const int IC_DIO_PIN = 18;
const int STEPPER_FOUR_PIN = 17;
const int STEPPER_THREE_PIN = 16;
const int STEPPER_TWO_PIN = 15;
const int STEPPER_ONE_PIN = 14;
const int LASER_PIN = 0;
const PHOTO_RES_PIN = A14;

const int SAT = 220;
const int UNLOCKED = 0;
const int LOCKED = 180;
const int STEPS_PER_REV = 2048;
const int STEPPER_SPEED = 5;
const int TOTAL_SLOTS = 7;
const int BEAM_BROKEN = 400;

const byte ROWS = 4;
const byte COLS = 4;

int codeIndex;
int rotation;
int digitsCorrect;
int stepCount;
int bulb;
int brightness;
int candySlot;
int mappedCandySlot;
int beamRead;

static int outlet;
static int firstDig;
static int secondDig;
static int thirdDig;
static int forthDig;

char customKey;
char candySelection;

bool status;
bool isLocked;
bool lock;
bool turnedOn;

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
Keypad
    customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

EthernetClient client;
IOTTimer trapDoorTimer;
Wemo wemo;
Stepper CandyStepper(stepsPerRevolution,
                     STEPPER_ONE_PIN,
                     STEPPER_TWO_PIN,
                     STEPPER_THREE_PIN,
                     STEPPER_FOUR_PIN);
TM1637 fourDigDisplay(IC_CLK_PIN, IC_DIO_PIN);

void setUpKeypad() {
  display.println("Serial Com established...");
  EthernetStatus = Ethernet.begin(mac);
  if (!EthernetStatus) {
    display.printf("failed to configure Ethernet using DHCP \n");
  }
  mappedCandySlot = map(candySlot, 0, TOTAL_SLOTS, 0, STEPS_PER_REV);
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

void setUpFourDigDisplay() {
  fourDigDisplay.init();
  fourDigDisplay.set(BRIGHT_TYPICAL);
}

void checkKeypad() {
  firstDig = random(0, 9);
  secondDig = random(0, 9);
  thirdDig = random(0, 9);
  forthDig = random(0, 9);
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
  showFourDig(codeIndex);
  codeIndex < 4 ? codeIndex++ : codeIndex = 3;
}

void checkCode() {
  for (int i = 0; i < codeIndex; i++) {
    isLocked = enteredCode[i] == defaultCode[i];
    if (isLocked == true) {
      digitsCorrect++;
    } else {
      digitsCorrect = 0;
    }
  }

  codeIndex = 0;
  if (digitsCorrect >= 4) {
    openDoor();
  }
  digitsCorrect = 0;
}

void showFourDig(int keypress) {
  switch (keypress) {
    case 0:
      fourDigDisplay.display(0, firstDig);
      fourDigDisplay.display(1, secondDig);
      fourDigDisplay.display(2, thirdDig);
      fourDigDisplay.display(3,
                             enteredCode[0] == null
                             ? forthDig
                             : enteredCode[0]);
      break;
    case 1:
      fourDigDisplay.display(0, firstDig);
      fourDigDisplay.display(1, secondDig);
      fourDigDisplay.display(2, enteredCode[0]);
      fourDigDisplay.display(3, enteredCode[1]);
      break;
    case 2:
      fourDigDisplay.display(0, firstDig);
      fourDigDisplay.display(1, enteredCode[0);
      fourDigDisplay.display(2, enteredCode[1]);
      fourDigDisplay.display(3, enteredCode[2]);
      break;
    case 3:
      fourDigDisplay.display(0, enteredCode[0]);
      fourDigDisplay.display(1, enteredCode[1]);
      fourDigDisplay.display(2, enteredCode[2]);
      fourDigDisplay.display(3, enteredCode[3]);
      break;
  }
}

void selectBulb(char bulb) {
  switch (bulb) {
    case 'A':
      setHue(0, false, 0, 0, 0);
      break;
    case 'B':
      setHue(1, false, 0, 0, 0);
      break;
    case 'C':
      setHue(2, false, 0, 0, 0);
      break;
    case 'D':
      setHue(3, false, 0, 0, 0)
      break;
  }
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
  OLED.pickCandy();
  rotateCandy();
  checkBeam();
  trapDoorServo.write(UNLOCKED);
  trapDoorTimer.startTimer(550);
  while (!trapDoorTimer.isReady);
  locking();
}

void nextOutlet() {
  if (outlet > 3) {
    outlet = 0;
  }
  wemo.turnOn(outlet);
  outlet++;
}

void setToGreen(int bulbCount) {
  turnedOn = true;
  for (int i = 0; i < bulbCount; i++) {
    brightness = (int) 127 * sin(2 * PI * .1 * now()) + 127;
    setHue(i, turnedOn, HueGreen, brightness, SAT);
  }
}

void lightLaser() {
  digitalWrite(LASER_PIN, HIGH);
}

void welcome() {
  OLED.clearDisplay();
  OLED.printf(
      "Welcome to the SmartSafe\nEnter your code\nOr\nSet the lights with *\n&\nA - D");
  OLED.display();
  setToGreen(3);
  lightLaser();
  wemo.turnOn(0); // Light to see the candy better
}

void rotateCandy() {
  candySelection = customKeypad.getKey();
  while (!candySelection);
  candyStepper.setSpeed(STEPPER_SPEED);
  switch (candySelection) {
    case '1':
      candySlot = 0;
      break;
    case '2':
      candySlot = 1;
      break;
    case '3':
      candySlot = 2;
      break;
    case '4':
      candySlot = 3;
      break;
    case '5':
      candySlot = 4;
      break;
    case '6':
      candySlot = 5;
      break;
    case '7':
      candySlot = 6;
      break;
    case '8':
      candySlot = 7;
      break;
  }
  showCandySelection();
  candyStepper.steps(mappedCandySlot);
}

void showCandySelection() {
  OLED.clearDisplay();
  OLED.setTextSize(4);
  OLED.print(candySlot);
  OLED.display();
}

void checkBeam() {
  beamRead = analogRead(PHOTO_RES_PIN);
  while (beamRead > BEAM_BROKEN);
}

#endif //SMARTSAFE_KEYPAD_H
