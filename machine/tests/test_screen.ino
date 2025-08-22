//OLED LCD サンプル
//SDA データ線 GPIO 21
//SCL クロック線GPIO 22

# include <Wire.h>
# include "SSD1306.h"//ディスプレイ用ライブラリを読み込み

//本体裏側　0x78に接続→0x3C 0x7A→0x3A
SSD1306  display(0x3c, 21, 22); //SSD1306インスタンスの作成（I2Cアドレス,SDA,SCL）


void setup() {
  display.init();    //ディスプレイを初期化
  display.setFont(ArialMT_Plain_16);    //フォントを設定
  display.drawString(0, 0, "Hello SSD1306!!");    //(0,0)の位置にHello Worldを表示
  display.display();   //指定された情報を描画
}

void loop() {

}
