//
// Created by Russell on 7/1/2021.
//

#ifndef SMARTSAFE_DISPLAY_H
#define SMARTSAFE_DISPLAY_H

/*
   Project: I2C scanner
   Description: Code to find I2C devices
   Author: Russell Brazell
   Date: 6-22-2021
*/

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

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
IOTTimer timer;

void setup() {
    Serial.begin(11520);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // Don't proceed, loop forever
    }
    setSyncProvider(getTeensy3Time);
    display.clearDisplay();
    Wire.begin();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setRotation(0);
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
    display.clearDisplay();
    currentHour = hour();
    currentMin = minute();
    currentDay = day();
    currentMnth = month();
    currentYear = year();
    resetCursor();
    digitalClockDisplay();
}

void resetCursor() {
    display.setCursor(0, 0);
}

void digitalClockDisplay() {
    // digital clock display of the time
    display.printf("%02i:%02i\n%02i-%02i-%04i\n",
                   currentHour, currentMin, currentMnth, currentDay, currentYear);
    display.display();
}

time_t getTeensy3Time() {
    return Teensy3Clock.get();
}

void pickCandy() {
    display.clearDisplay();
    display.printf("Choose\nWisely");
    display.setTextSize(3);
    display.display();
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

#endif //SMARTSAFE_DISPLAY_H
