/*
 * Project: Smart Safe
 * Description: Code to control my smart safe
 * Author: Russell Brazell
 * Date: 6-28-2021
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IOTTimer.h>
#include <TimeLib.h>
#include <math.h>
#include <PWMServo.h>
#include <Keypad.cpp>
#include <IOTTimer.h>
#include <Ethernet.h>
#include <Mac.h>
#include <Wemo.h>
#include <Stepper.h>

//PINS
const int SERVO_TRAP_DOOR_PIN = 23;
const int SERVO_VAULT_DOOR_PIN = 22;
const int TRIG_PIN = 21;
const int ECHO_PIN = 20;
const int STEPPER_ONE_PIN = 14;
const int STEPPER_TWO_PIN = 15;
const int STEPPER_THREE_PIN = 16;
const int STEPPER_FOUR_PIN = 17;
const int LASER_PIN = 0;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET = 4;
const int SCREEN_ADDRESS = 0x3C;
const int UNLOCKED = 0;
const int LOCKED = 180;
const int STEPS_PER_REV = 2048;

int currentHour = hour();
int currentMin = minute();
int currentDay = day();
int currentMnth = month();
int currentYear = year();
int codeIndex;
int rotation;
int digitsCorrect;
int stepCount;

static int outlet;

float distance;
float echoTime;
float calculatedDistance;

char customKey;
char bulbSelect;

char defaultCode[] = {'1', '1', '1', '1'};
char enteredCode[4];
char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

byte i;
byte count;

byte rowPins[ROWS] = {9, 8, 7, 6}; // keypad leads 8 , 7 , 6 , 5
byte colPins[COLS] = {5, 4, 3, 2}; // keypad leads 4 , 3 , 2 , 1

bool detected;
bool status;
bool isLocked;
bool lock;

Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
IOTTimer OLEDtimer;
PWMServo trapDoorServo;
PWMServo vaultDoorServo;
Keypad
    customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
EthernetClient client;
Wemo wemo;
Stepper CandyStepper(stepsPerRevolution,
                     STEPPER_ONE_PIN,
                     STEPPER_TWO_PIN,
                     STEPPER_THREE_PIN,
                     STEPPER_FOUR_PIN);


void setup() {
  setUpOLED();
  setUpUSSensor();
  setUpKeypad();
}

void loop() {
  if (isDetect) {
    welcome();
  } else {
    OLEDLoop();
  }
  checkKeypad();
}

void setUpOLED() {
  Serial.begin(11520);
  if (!OLED.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  setSyncProvider(getTeensy3Time);
  OLED.clearDisplay();
  Wire.begin();
  OLED.setTextSize(1);
  OLED.setTextColor(SSD1306_WHITE);
  OLED.setRotation(0);
  resetCursor();
}

void setUpUSSensor() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

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

void OLEDLoop() {
  OLEDtimer.startTimer(1000);
  if (Serial.available()) {
    time_t t = processSyncMessage();
    if (t != 0) {
      Teensy3Clock.set(t); // set the RTC
      setTime(t);
    }
  }
  while (!OLEDtimer.isTimerReady());
  OLED.clearDisplay();
  currentHour = hour();
  currentMin = minute();
  currentDay = day();
  currentMnth = month();
  currentYear = year();
  resetCursor();
  digitalClockDisplay();
}

void resetCursor() {
  OLED.setCursor(0, 0);
}

void digitalClockDisplay() {
  // digital clock display of the time
  OLED.printf("%02i:%02i\n%02i-%02i-%04i\n",
              currentHour, currentMin, currentMnth, currentDay, currentYear);
  OLED.display();
}

bool isDetect() {
  detected = false;
  if (getDistance() != 0) { // TODO set to 3 feet
    detected = true;
  }
  return detected;
}

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

float getDistance() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  echoTime = pulseIn(ECHO_PIN, HIGH, 250L);
  return calculatedDistance = (echoTime / 148.0);
}

void checkKeypad() {
  customKey = customKeypad.getKey();
  if (customKey) {
    if (customKey == '*') {
bulbSelect = customKeypad.getKey();
      while (!bulbSelect);
      switch (bulbSelect) {
        case 'A':
//          TODO turn off 1 bulb
          break;

      }
    } else {
      enterCode();
      if (codeIndex == 4) {
        checkCode();
      }
    }
  }
}

void enterCode() {
  enteredCode[codeIndex] = customKey;
  codeIndex < 4 ? codeIndex++ : codeIndex = 3;
}

void checkCode() {
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

void pickCandy() {
  OLED.clearDisplay();
  OLED.setTextSize(2);
  OLED.printf("Pick Your\nCandy");
  OLED.display();
}

void lightLaser() {
  digitalWrite(LASER_PIN, HIGH);
}

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message


unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if (Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    return pctime;
  }
  return pctime;
}