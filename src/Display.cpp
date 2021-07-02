
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IOTTimer.h>
#include <TimeLib.h>

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int OLED_RESET = 4;
const int SCREEN_ADDRESS = 0x3C;

int currentHour = hour();
int currentMin = minute();
int currentDay = day();
int currentMnth = month();
int currentYear = year();

byte i;
byte count;

Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
IOTTimer OLEDtimer;

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

void loop() {
    timer.startTimer(9000);
    if (Serial.available()) {
        time_t t = processSyncMessage();
        if (t != 0) {
            Teensy3Clock.set(t); // set the RTC
            setTime(t);
        }
    }
    while (!timer.isTimerReady());
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

time_t getTeensy3Time() {
    return Teensy3Clock.get();
}

void pickCandy() {
    OLED.clearDisplay();
    OLED.printf("Pick Your\nCandy");
    OLED.setTextSize(2);
    OLED.display();
}

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message


unsigned long processSyncMessage() {
    unsigned long pctime = 0L;
    const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

    if (Serial.find(TIME_HEADER)) {
        pctime = Serial.parseInt();
        return pctime;
        if ( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
            pctime = 0L; // return 0 to indicate that the time is not valid
        }
    }
    return pctime;
}
