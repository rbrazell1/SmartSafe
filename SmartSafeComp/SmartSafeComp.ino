#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IOTTimer.h>
#include <TimeLib.h>
#include <math.h>
#include <PWMServo.h>
#include <Keypad.h>
#include <IOTTimer.h>
#include <Ethernet.h>
#include <Mac.h>
#include <Wemo.h>
#include <Stepper.h>
#include <Hue.h>
#include <TM1637.h>

//PINS
const int SERVO_TRAP_DOOR_PIN = 23;
const int SERVO_VAULT_DOOR_PIN = 22;
const int TRIG_PIN = 21;
const int ECHO_PIN = 20;
const int IC_CLK_PIN = 19;
const int IC_DIO_PIN = 18;
const int STEPPER_FOUR_PIN = 17;
const int STEPPER_THREE_PIN = 16;
const int STEPPER_TWO_PIN = 15;
const int STEPPER_ONE_PIN = 14;
const int LASER_PIN = 0;
const int PHOTO_RES_PIN = A14;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET = 4;
const int SCREEN_ADDRESS = 0x3C;
const int SAT = 220;
const int UNLOCKED = 0;
const int LOCKED = 180;
const int STEPS_PER_REV = 2048;
const int STEPPER_SPEED = 5;
const int TOTAL_SLOTS = 7;
const int BEAM_BROKEN = 400;

const byte ROWS = 4;
const byte COLS = 4;

static int outlet;
static int firstDig;
static int secondDig;
static int thirdDig;
static int forthDig;

int currentHour = hour();
int currentMin = minute();
int currentDay = day();
int currentMnth = month();
int currentYear = year();
int codeIndex;
int rotation;
int digitsCorrect;
int stepCount;
int attempt;
int bulb;
int brightnessLevel;
int candySlot;
int mappedCandySlot;
int beamRead;

float distance;
float echoTime;
float calculatedDistance;

char customKey;
char candySelection;
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
bool EthernetStatus;
bool isLocked;
bool lock;

Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
IOTTimer OLEDtimer;
IOTTimer trapDoorTimer;
PWMServo trapDoorServo;
PWMServo vaultDoorServo;
Keypad
    customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

EthernetClient client;
Wemo wemo;
Stepper candyStepper(STEPS_PER_REV,
                     STEPPER_ONE_PIN,
                     STEPPER_TWO_PIN,
                     STEPPER_THREE_PIN,
                     STEPPER_FOUR_PIN);
TM1637 fourDigDisplay(IC_CLK_PIN, IC_DIO_PIN);

void setup() {
  setUpOLED();
  setUpUSSensor();
  setUpKeypad();
  setUpFourDigDisplay();
}

void loop() {
  checkKeypad();
  if (isDetect()) {
    welcome();
  } else {
    OLEDLoop();
  }
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
  OLED.printf("Serial Com established...\n");
  OLED.display();
  EthernetStatus = Ethernet.begin(mac);
  if (!EthernetStatus) {
    OLED.printf("failed to configure Ethernet using DHCP \n");
    OLED.display();
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
    if (customKey == '*') {
      while (!bulbSelect) {
        bulbSelect = customKeypad.getKey();
      }
      selectBulb(bulbSelect);
    } else {
      enterCode();
      if (codeIndex == 4) {
        checkCode();
      }
    }
  }
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

void setToGreen(int bulbCount) {
  for (int i = 0; i < bulbCount; i++) {
    brightnessLevel = (int) 127 * sin(2 * PI * .1 * now()) + 127;
    setHue(i, true, HueGreen, brightnessLevel, SAT);
  }
}

void lightLaser() {
  digitalWrite(LASER_PIN, HIGH);
}

float getDistance() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  echoTime = pulseIn(ECHO_PIN, HIGH, 250L);
  return calculatedDistance = (echoTime / 148.0);
}

bool isDetect() {
  detected = false;
  if (getDistance() != 0) { // TODO set to 3 feet
    detected = true;
  }
  return detected;
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
      setHue(3, false, 0, 0, 0);
      break;
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
  } else {
    setHue(attempt, true, HueRed, 210, 220);
    attempt++;
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
  checkBeam();
  trapDoorServo.write(UNLOCKED);
  trapDoorTimer.startTimer(550);
  while (!trapDoorTimer.isTimerReady());
  locking();
}

void pickCandy() {
  OLED.clearDisplay();
  OLED.setTextSize(2);
  OLED.printf("Pick Your\nCandy");
  OLED.display();
  rotateCandy();
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
  candyStepper.step(mappedCandySlot);
}

void checkBeam() {
  beamRead = analogRead(PHOTO_RES_PIN);
  while (beamRead > BEAM_BROKEN);
}

void showCandySelection() {
  OLED.clearDisplay();
  OLED.setTextSize(4);
  OLED.print(candySlot);
  OLED.display();
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

time_t getTeensy3Time() {
  return Teensy3Clock.get();
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
