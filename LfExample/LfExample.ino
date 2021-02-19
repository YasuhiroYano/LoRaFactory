/*  LoRafactory サンプルスケッチ
   　Ver1.0　2021/01/15 KK.YES 矢野
   This library is only for Uno like bord TYPE 3276-500.
   Made by http://www.kkyes.co.jp/
   Private LoRa module EASEL ES920LR https://easel5.com/
   Bord:Arduino Pro or Pro mini ATMega328P (3.3V8MHz)
*/
#include "LoRafactory.h"
#define CODINATOR 1
#if CODINATOR
LoRafactory Lf(1, 1, 0, 2, 2);  //cordinator
#else
LoRafactory Lf(0, 1, 0x1234, 2, 2); //End device
//(cordinator,panid,ownid,receiveNum,transmitNum);
#endif

void setup() {
  Serial.begin(115200);
#if CODINATOR
  if (Lf.connect(0x1234))Lf.setled(2);//送信先を指定して接続
#else
  if (Lf.connect(0))Lf.setled(3);//送信先を指定して接続
#endif
  pinMode(8, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
#if CODINATOR
  Serial.println("This is the Cordinator.");
#else
  Serial.println("This is a EndDevice.");
#endif

}

void loop() {
  static int i;
  String s;
#if CODINATOR
  //  if (Lf.recieve(&s)) {//文字列を受信
  //    Serial.println(s);
  if (Lf.recieve()) {//データ配列を受信
    Serial.print("0x");
    Serial.print(Lf.recid(), HEX);
    Serial.print(",");
    Serial.print((int)Lf.rssi());
    Serial.print(",");
    Serial.print(Lf.get_data(0));
    Serial.print(",");
    Serial.println(Lf.get_data(1));
    Lf.setled(0);//文字列を受信
  }
#else
  Lf.set_data(0, i++);
  Lf.set_data(1, -i + 20);
  if (Lf.transmit())Lf.setled(0); //データ配列を送信
  //  if (Lf.transmit("this is endterminal"))Lf.setled(0);//文字列を送信
  delay(1000);//短くすると受信文字が欠落する
#endif
}
