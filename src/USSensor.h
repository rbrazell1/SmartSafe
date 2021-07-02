//
// Created by russell on 7/2/2021.
//

#ifndef SMARTSAFE_USSENSOR_H
#define SMARTSAFE_USSENSOR_H

const int TRIG_PIN = 21;
const int ECHO_PIN = 20;

float distance;
float echoTime;
float calculatedDistance;

bool detected;

void setUpUSSensor() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
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
#endif //SMARTSAFE_USSENSOR_H
