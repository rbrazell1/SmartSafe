#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <Adafruit_MPU6050.h>
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
#include <HCSR04.h>

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
const int MPU_ADDR = 0x68;
const int SAT = 220;
const int UNLOCKED = 160;
const int LOCKED = 0;
const int OPEN_TRAP = 100;
const int CLOSE_TRAP = 0;
const int STEPS_PER_REV = 2048;
const int STEPPER_SPEED = 5;
const int TOTAL_SLOTS = 7;
const int BEAM_BROKEN = 210;
const int CODE_LENGTH = 4;
const int DETECT_DIST = 40;
const int HUE_COUNT = 5;


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
int lastSlot;
int currentSlot;
int finalSlot;
int beamRead;
int distance;
int yShake;
int zShake;

unsigned long echoTime;

char customKey;
char candySelection;
char bulbSelect;
char wemoSelect;

char defaultCode[] = {'1', '1', '1', '1'};
char enteredCode[CODE_LENGTH];
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
char tmp_str[7];

byte i;
byte count;

byte rowPins[ROWS] = {8, 7, 6, 5}; // keypad leads 8 , 7 , 6 , 5
byte colPins[COLS] = {4, 3, 2, 1}; // keypad leads 4 , 3 , 2 , 1

bool detected;
bool EthernetStatus;
bool isLocked;
bool lock;
bool welcomed;
bool power1;
bool power2;
bool power3;
bool power4;
bool power5;
bool switch1;
bool switch2;
bool switch3;
bool switch4;

int16_t accelerometer_y;
int16_t accelerometer_z;

Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
IOTTimer OLEDtimer;
IOTTimer trapDoorTimer;
IOTTimer shakeTimer;
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
HCSR04 ultraSonic(TRIG_PIN, ECHO_PIN);

void setup() {
  Serial.begin(115200);
  //  while (!Serial);
  setUpOLED();
  setUpKeypad();
  setUpFourDigDisplay();
  setUpAccel();
  Serial.println("Setup complete");
}

void loop() {
  if (isDetect() && welcomed == false) {
    welcome();
    Serial.println("step1A");
    welcomed = true;
  } else {
    OLEDLoop();
    checkKeypad();
  }
}

void setUpOLED() {
  Wire.begin();
  OLED.begin();
  if (!OLED.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }

  setSyncProvider(getTeensy3Time);
  OLED.clearDisplay();
  OLED.display();
  OLED.setTextSize(1);
  OLED.setTextColor(SSD1306_WHITE);
  OLED.setRotation(2);
  resetCursor();
  welcomed = false;
}

void setUpKeypad() {
  OLED.printf("Serial Com established...\n");
  OLED.display();
  EthernetStatus = Ethernet.begin(mac);
  if (!EthernetStatus) {
    OLED.clearDisplay();
    OLED.printf("failed to configure Ethernet using DHCP \n");
    OLED.display();
  }
  trapDoorServo.attach(SERVO_TRAP_DOOR_PIN);
  vaultDoorServo.attach(SERVO_VAULT_DOOR_PIN);
  codeIndex = 0;
  digitsCorrect = 0;
  pinMode(10, OUTPUT);
  //  pinMode(4, OUTPUT);
  digitalWrite(10, HIGH);
  //  digitalWrite(4, HIGH);
  outlet = 0;
  OLED.clearDisplay();
  OLED.display();
  resetCursor();
  setToGreen(HUE_COUNT);
}

void setUpFourDigDisplay() {
  fourDigDisplay.init();
  fourDigDisplay.set(3);
}

void setUpAccel() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void checkKeypad() {
  firstDig = random(0, 9);
  secondDig = random(0, 9);
  thirdDig = random(0, 9);
  forthDig = random(0, 9);
  Serial.printf("testBEFORE\n");
  customKey = customKeypad.getKey();
  while (!customKey) {
    customKey = customKeypad.getKey(); 
//    checkShake();
  }
  if (customKey == '*') {
    while (!bulbSelect) {
      bulbSelect = customKeypad.getKey();
//      checkShake();
    }
    selectBulb(bulbSelect);
    bulbSelect = 0x00;
  } if (customKey == '#') {
    while (!wemoSelect) {
      wemoSelect = customKeypad.getKey();
//      checkShake();
    }
    selectWemo(wemoSelect);
    wemoSelect = 0x00;
  } else
    enterCode();
  if (codeIndex == CODE_LENGTH) {
    checkCode();
  }
}

void welcome() {
  OLED.clearDisplay();
  OLED.display();
  OLED.printf(
    "Welcome to the SmartSafe\nEnter your code\nOr\nSet the lights with *\n&\nA - D\n");
  OLED.display();
  setToGreen(HUE_COUNT);
  lightLaser();
  wemo.turnOn(0); // Light to see the candy better
}

void OLEDLoop() {
  if (Serial.available()) {
    time_t t = processSyncMessage();
    if (t != 0) {
      Teensy3Clock.set(t); // set the RTC
      setTime(t);
    }
  }
  OLED.clearDisplay();
  OLED.display();
  currentHour = hour();
  currentMin = minute();
  currentDay = day();
  currentMnth = month();
  currentYear = year();
  resetCursor();
  digitalClockDisplay();
  Serial.println("Finished OLED loop");
}

void setToGreen(int bulbCount) {
  for (int i = 0; i <= bulbCount; i++) {
    brightnessLevel = (int) 127 * sin(2 * PI * .1 * now()) + 127;
    setHue(i, true, HueGreen, brightnessLevel, SAT);
  }
  attempt = 1;
}

void setToRed(int bulbCount) {
  for (int i = 0; i <= bulbCount; i++) {
    brightnessLevel = (int) 127 * sin(2 * PI * .1 * now()) + 127;
    setHue(i, true, HueRed, brightnessLevel, SAT);
  }
  attempt = 1;
}

void lightLaser() {
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, HIGH);
}

int getDistance() {
  return distance = ultraSonic.dist();
}

bool isDetect() {
  detected = false;
  if (getDistance() <= DETECT_DIST) {
    detected = true;
  }
  return detected;
}

void selectBulb(char _bulb) {
  switch (_bulb) {
    case 'A':
      setHue(1, power1, 0, 0, 0);
      power1 = !power1;
      break;
    case 'B':
      setHue(2, power2, 0, 0, 0);
      power2 = !power2;
      break;
    case 'C':
      setHue(3, power3, 0, 0, 0);
      power3 = !power3;
      break;
    case 'D':
      setHue(4, power4, 0, 0, 0);
      power4 = !power4;
      break;
    case '0':
      setHue(5, power5, 0, 0, 0);
      power5 = !power5;
      break;
  }
}

void selectWemo(char _wemo) {
  switch (_wemo) {
    case 'A':
      switch1 = !switch1;
      switch1
      ? wemo.turnOn(0)
      : wemo.turnOff(0);
      break;
    case 'B':
      switch2 = !switch2;
      switch2
      ? wemo.turnOn(1)
      : wemo.turnOff(1);
      break;
    case 'C':
      switch3 = !switch3;
      switch3
      ? wemo.turnOn(2)
      : wemo.turnOff(2);
      break;
    case 'D':
      switch4 = !switch4;
      switch4
      ? wemo.turnOn(3)
      : wemo.turnOff(3);
      break;
  }
}

void enterCode() {
  enteredCode[codeIndex] = customKey;
  showFourDig(codeIndex);
  if (codeIndex <= CODE_LENGTH) {
    codeIndex++;
  }
  Serial.printf("testCODEINDEX: %i\n", codeIndex);
}

void checkCode() {
  for (int i = 0; i < CODE_LENGTH; i++) {
    isLocked = enteredCode[i] == defaultCode[i];
    isLocked == true
    ? digitsCorrect++
    : digitsCorrect = 0;
  }
  codeIndex = 0;
  if (digitsCorrect == CODE_LENGTH) {
    openDoor();
  } else {
    setHue(attempt, true, HueRed, 210, 220);
    attempt++;
  }
  digitsCorrect = 0;
}

void openDoor() {
  Serial.println("unlocking");
  unlocking();
  digitsCorrect = 0;
  locking();
}

void locking() {
  trapDoorServo.write(CLOSE_TRAP);
  vaultDoorServo.write(LOCKED);
}

void unlocking() {
  vaultDoorServo.write(UNLOCKED);
  pickCandy();
  checkBeam();
  trapDoorServo.write(OPEN_TRAP);
  trapDoorTimer.startTimer(550);
  Serial.println("Waiting to close");
  while (!trapDoorTimer.isTimerReady());
  setToGreen(HUE_COUNT);
}

void pickCandy() {
  OLED.clearDisplay();
  OLED.display();
  OLED.setTextSize(2);
  OLED.printf("Pick Your\nCandy");
  OLED.display();
  rotateCandy();
}

void rotateCandy() {
  candySelection = customKeypad.getKey();
  while (!candySelection) {
    candySelection = customKeypad.getKey();
  }
  candyStepper.setSpeed(STEPPER_SPEED);
  Serial.println("Ready to move stepper");
  switch (candySelection) {
    case '1':
      candySlot = 1;
      break;
    case '2':
      candySlot = 2;
      break;
    case '3':
      candySlot = 3;
      break;
    case '4':
      candySlot = 4;
      break;
    case '5':
      candySlot = 5;
      break;
    case '6':
      candySlot = 6;
      break;
    case '7':
      candySlot = 7;
      break;
    case '8':
      candySlot = 8;
      break;
  }
  showCandySelection();
  mappedCandySlot = map(candySlot, 0, TOTAL_SLOTS, 0, STEPS_PER_REV);
  candyStepper.step(-mappedCandySlot);
}

void checkBeam() {
  beamRead = analogRead(PHOTO_RES_PIN);
  Serial.printf("BEAM READ: %i\n", beamRead);
  while (beamRead > BEAM_BROKEN) {
    beamRead = analogRead(PHOTO_RES_PIN);
  }
}

void showCandySelection() {
  OLED.clearDisplay();
  OLED.display();
  OLED.setTextSize(4);
  OLED.print(candySlot);
  OLED.display();
}


void resetCursor() {
  OLED.setCursor(0, 0);
}

void showFourDig(int keypress) {
  switch (keypress) {
    case 0:
      fourDigDisplay.display(0, firstDig);
      fourDigDisplay.display(1, secondDig);
      fourDigDisplay.display(2, thirdDig);
      fourDigDisplay.display(3,
                             enteredCode[0] == 0x00
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
      fourDigDisplay.display(1, enteredCode[0]);
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

char* convert_int16_to_str(int16_t i) {
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

void accelRead() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 7 * 2, true); // request a total of 7*2=14 registers

  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  accelerometer_y = Wire.read() << 8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z = Wire.read() << 8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)

  // print out data
  Serial.print("|aY= "); Serial.print(convert_int16_to_str(accelerometer_y));
  Serial.print(" |aZ="); Serial.print(convert_int16_to_str(accelerometer_z));
}

void checkShake() {
  accelRead();
  yShake = (int)convert_int16_to_str(accelerometer_y); // TODO find a string to int to set correctly
  zShake = (int)convert_int16_to_str(accelerometer_z);
  Serial.printf("Y shake: %i\nZ shake: %i\n", yShake, zShake);
  if (yShake > 3000 || yShake < -3000 || zShake > 3000 || zShake < -3000) {
    for (int j = 0; j < 4; j++) {
      setToRed(HUE_COUNT);
      shakeTimer.startTimer(500);
      while (!shakeTimer.isTimerReady());
      wemo.turnOff(j);
      setToGreen(HUE_COUNT);
    }
  }
}

void digitalClockDisplay() {
  // digital clock display of the time
  OLED.setTextSize(2);
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
