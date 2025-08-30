/* Example sketch to control a stepper motor with TB6600 stepper motor driver 
  and Arduino without a library: continuous rotation. 
  More info: https://www.makerguides.com */

// Define stepper motor connections:
#define dirPin 25
#define stepPin 26
// Pin for check button
#define checkButtonPin 1
# include <Wire.h>
# include "SSD1306.h"//ディスプレイ用ライブラリを読み込み

SSD1306  display(0x3c, 33, 32); //SSD1306インスタンスの作成（I2Cアドレス,SDA,SCL）

void setup_screen() {
  display.init();    //ディスプレイを初期化
  display.setFont(ArialMT_Plain_16);    //フォントを設定
  display.drawString(0, 0, "Hello SSD1306!!");    //(0,0)の位置にHello Worldを表示
  display.display();   //指定された情報を描画
}
#define tempUpButton 27
#define tempDownButton 14

void button_setup() {
  pinMode(checkButtonPin, INPUT_PULLUP);
  pinMode(tempUpButton , INPUT_PULLUP);
  pinMode(tempDownButton, INPUT_PULLUP);
}
void setup() {
  Serial.begin(115200);
  // Declare pins as output:
  stepper_setup();
  button_setup();
  setup_screen();
  calibrate_stepper_motor();
}
void loop() {
  delay(50);
  if (digitalRead(tempUpButton) == 0) {
    move_motor(true, 200);
    display.clear();
    display.drawString(0, 0, "Up");
    display.display();   //指定された情報を描画
    Serial.println("fuck");
  } else if (digitalRead(tempDownButton) == 0) {
    move_motor(false,200);
    display.clear();
    display.drawString(0, 0, "Down");
    display.display();   //指定された情報を描画
    Serial.println("fuck2");
  }
}
void stepper_setup() {
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
}

void move_motor(bool dir, int steps) {
    // Set direction
    digitalWrite(dirPin, dir ? HIGH : LOW);

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
