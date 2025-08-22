#define checkButtonPin 25;  // the number of the pushbutton pin

void setup() {
    Serial.begin(9600);
    button_setup();
}

void button_setup() {
    pinMode(checkButtonPin, INPUT_PULLUP);
}

void loop() {
    if (digitalRead(checkButtonPin) == LOW) { // since it is pulling up
        Serial.println("check button is pressed");
    }
}
