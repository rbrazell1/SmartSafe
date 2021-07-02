
#include <Stepper.h>

const int stepsPerRevolution = 2048;

int stepCount = 0;

Stepper CandyStepper(stepsPerRevolution, 8, 9, 10, 11);

