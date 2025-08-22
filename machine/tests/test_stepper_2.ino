/* Example sketch to control a stepper motor with TB6600 stepper motor driver 
  and Arduino without a library: continuous rotation. 
  More info: https://www.makerguides.com */

// Define stepper motor connections:
#define dirPin 32
#define stepPin 33
// Pin for check button
#define checkButtonPin 25

void setup() {
    // Declare pins as output:
    stepper_setup();
    button_setup();

    calibrate_stepper_motor();
}

void stepper_setup() {
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
}

void button_setup() {
    pinMode(checkButtonPin, INPUT_PULLUP);
}

void move_motor(bool dir, int steps) {
    digitalWrite(dirPin, dir);
    // These four lines result in 1 step:wwww
    for (int i = 0; i < steps; i++) {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(500);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(500);
    }
}

bool calibrate_stepper_motor() {
    while (digitalRead(checkButtonPin) == HIGH) {
        move_motor(0, 1);
    }
    return true;
}
