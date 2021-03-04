/*  LoRafactory サンプルスケッチ
   Ver1.0　2021/01/15 KK.YES 矢野
   Ver1.1  2021/01/29 KK.YES 矢野 ランダムに送信遅延を行う
   Ver1.2 2021/03/02 KK.YES 矢野　LED点滅処理を割り込みで行う   
   This library is only for Uno like bord TYPE 3276-500.
   Made by http://www.kkyes.co.jp/
   Private LoRa module EASEL ES920LR https://easel5.com/
   Bord:Arduino Pro or Pro mini ATMega328P (3.3V8MHz)
*/
#include "LoRafactory.h"
#include <MsTimer2.h>
#define CODINATOR 0
int EndDeviceId = 6;

int CordinatorId = 0;
int LoopTime = 500;//30sec

#if CODINATOR
LoRafactory Lf(1, 1, CordinatorId, 2, 2);  //cordinator
#else
LoRafactory Lf(0, 1, EndDeviceId, 2, 2); //End device
//(cordinator,panid,ownid,receiveNum,transmitNum);
#endif

void setup() {
  Serial.begin(115200);
#if CODINATOR
  if (Lf.connect(EndDeviceId))Lf.setled(2);//送信先を指定して接続
#else
  if (Lf.connect(CordinatorId))Lf.setled(3);//送信先を指定して接続
#endif
  pinMode(8, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
#if CODINATOR
  Serial.println("This is the Cordinator.");
#else
  Serial.println("This is a EndDevice.");
  randomSeed(EndDeviceId);
  delay(random(LoopTime));
#endif
  MsTimer2::set(10, timerJob);              //10ミリ毎のタイマーイベント
  MsTimer2::start();
}


void loop() {
  static int i;
  String s;
  int ltime;
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
    Lf.ledpattern(1);//led点灯パターンを設定
  }
#else
  Lf.set_data(0, i++);
  Lf.set_data(1, -i + 20);
  if (Lf.transmit()) {
    Lf.ledpattern(1);//led点灯パターンを設定
    delay(LoopTime);
  }
  else {
    ltime = random(LoopTime);
    Serial.print("Fail dly=");
    Serial.println(ltime);
    delay(ltime);
  }
  //  if (Lf.transmit("this is endterminal"))Lf.setled(0);//文字列を送信
#endif
}
//---------ﾀｲﾏｰ割り込みｲﾍﾞﾝﾄ-----------
void timerJob(void) {
  Lf.ledjob();
}
