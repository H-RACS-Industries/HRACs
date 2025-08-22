#define checkButtonPin 25 // the number of the pushbutton pin
#define tempUpButton 19
#define tempDownButton 18

void setup() {
    Serial.begin(9600);
    button_setup();
}

void button_setup() {
    pinMode(checkButtonPin, INPUT_PULLUP);
    pinMode(tempUpButton , INPUT_PULLUP);
    pinMode(tempDownButton, INPUT_PULLUP);
}

void loop() {
    Serial.println(
    String("Check:") + digitalRead(checkButtonPin) + " " +
    "Up: " + digitalRead(tempUpButton) + " " +
    "Down: " + digitalRead(tempDownButton)
    );
}
