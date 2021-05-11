/*  LoRafactory イノシシ罠中継器　
     LfRepeater
   Ver1.0  2021/04/27 KK.YES 明石 子機中継機能作成

   This library is only for Uno like bord TYPE 3276-500.
   Made by http://www.kkyes.co.jp/
   Private LoRa module EASEL ES920LR https://easel5.com/
   Bord:Arduino Pro or Pro mini ATMega328P (3.3V8MHz)
*/
#include "LoRafactory.h"

int MyId = 0x300;
int DestId = 0;   //ｹﾞｰﾄｳｪｲID

LoRafactory Lf(0, 10, MyId, 2, 7); //End device
//(cordinator,panid,ownid,receiveNum,transmitNum);
float BattVolt;//バッテリ電圧　0.01V
int SolarCur;//太陽電池電流　mA

#define CONNECT() Lf.connect(DestId, 3, 10)

void setup() {
  Serial.begin(115200);
  if (CONNECT())Lf.setled(3); //送信先を指定して接続
  Serial.println("A Repeater for Boar Trap.v1.0");
}

void loop() {

  if (Lf.recieve()) {//データ配列を受信
      SendRevData();  //受信ﾃﾞｰﾀを転送       
  }  
}

void SendRevData(){
  
  switch (Lf.command()) {
    case 'T'://Ambientデータ中継転送
      SendData('P');
      break;
    case 'L'://LINEメッセージ中継転送
      SendData('C');
      break;
    default:
      break;      
  }
}

void Measure() {//ｽﾃｰﾀｽ送信時に呼ばれる
  long d;
  d = analogRead(1);
  BattVolt = (float)d * 19.80 / 1024; //バッテリ電圧　V
  d = analogRead(0);
  SolarCur = d * 450 / 1024;   //太陽電池電流　mA
}


void SendData(char cmd) {//P:Ambient RSSI データ中継　C:LINEにメッセージ中継
  Measure();  //測定データ取得
  
  Lf.command(cmd);
  Lf.set_data(0, Lf.get_data(0));
  Lf.set_data(1, Lf.get_data(1));
  Lf.set_data(2, Lf.get_data(2));
  Lf.set_data(3, Lf.recid());
  Lf.set_data(4, BattVolt);
  Lf.set_data(5, SolarCur);
  Lf.set_data(6, Lf.rssi());
  if (Lf.transmit()) {
    Lf.setled(1);
  } else Lf.setled(0);

  Serial.print(cmd);
  Serial.print(",");
  Serial.print(Lf.get_data(0));
  Serial.print(",");
  Serial.print(Lf.get_data(1));
  Serial.print(",");
  Serial.print(Lf.get_data(2));
  Serial.print(",");
  Serial.print(Lf.recid());
  Serial.print(",");
  Serial.print(BattVolt);
  Serial.print(",");
  Serial.print(SolarCur);
  Serial.print(",");
  Serial.println(Lf.rssi());
}
