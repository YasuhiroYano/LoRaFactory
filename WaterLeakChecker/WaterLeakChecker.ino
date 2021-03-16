/*  LoRafactoryを使用した　漏水監視
   Ver1.0　2021/02/07 KK.YES 矢野
   Ver1.1 2021/02/17 KK.YES 矢野 電流計測感度変更
   Ver1.2 2021/03/13 KK.YES 矢野 帯域幅、拡散係数変更に伴う変更
   This library is only for Uno like bord TYPE 3276-500.
   Made by http://www.kkyes.co.jp/
   Private LoRa module EASEL ES920LR https://easel5.com/
   Bord:Arduino Pro or Pro mini ATMega328P (3.3V8MHz)
*/
#define OWNID 0x100 //LoRa端末Id　１～0xFFFF
#include <avr/wdt.h>
#include "LoRafactory.h"
LoRafactory Lf(0, 10, OWNID, 0, 4); //End device テキストで交信
//(cordinator,panid,ownid,receiveNum,transmitNum);

#define TX_INTERVAL 120//通信間隔　2分
#define TIMLEAK 60 //漏水監視時間　2時間
#define TIMNOFLOW 450 //無使用監視時間　15時間
#define CONNECT() Lf.connect(0,3,10) //bw=3(62.5k) sf=10

volatile unsigned int CountWater;//水量計パイロット回転を割り込みでカウント
//パルス周波数　546Hzまでオーバーフローしないはず。　実際は100Hz以下と思われる
int Status;//状態LINEにメッセージ番号　0:正常　1:漏水　2:無使用
float BattVolt;//バッテリ電圧　0.01V
int SolarCur;//太陽電池電流　mA
int WaterFlow;//水量　
int TimLeak, TimNoFlow; //漏水監視時間、無使用監視時間
int t; //1sec time count
int LedPin = 7;
int PlusTest;//0:通常　1:流水パルスごとにLEDを点滅
int RssiTest;//RSSIを要求中

void setup() {
  delay(1000);
  Serial.begin(57600);//115200bauは誤差がある　
  pinMode(LedPin, OUTPUT);
  Lf.setled(LedPin, 3);
  //  if (Lf.connect(0))Lf.setled(LedPin, 3); //送信先を指定して接続
  Serial.println("WaterLeak checker v1.00 ");
  attachInterrupt(1, IntWaterPlse, FALLING);//D3 割り込みで使用
//  wdt_enable(WDTO_8S);//wdtを有効にするとフリーズする。　
//電源を入れなおさないとプログラムの書き込みもできない状態になる。
  PlusTest = 0; //Normal
}

void loop() {
  static int i;
  String s = "";
  char c;
//  wdt_reset();
  if (t % 10 == 0)   Lf.setled(LedPin, 0);
  while (Serial.available()) {
    c = Serial.read();
    s += c;
  }
  if (s.length()) {
    PlusTest = 0;
    s.toUpperCase();
    switch (s[0]) {
      case 'R'://RSSIを要求する
        RssiTest = 1;
        CONNECT();
        Lf.command('R');
        if (Lf.transmit())Lf.setled(LedPin, 1); //RSSIを送り返してもらう
        break;
      case 'P'://パルステストモードに切り替える
        Serial.println("Pulse Test");//パルスでLEDが点滅する
        PlusTest = 1;
        break;
      case 'L'://試験的にLINEにメッセージを送る
        Serial.println("LINE を送ります");
        SendData('L');
        break;
      case 'N'://未使用継続の試験
        TimLeak = 1 - TIMNOFLOW;
        Serial.print("未使用継続の試験 Timleak=");
        Serial.println(TimLeak);
        break;
      case 'C'://漏水検知試験
        TimLeak = TIMLEAK - 1;
        Serial.print("漏水検知試験 Timleak=");
        Serial.println(TimLeak);
        break;
      default:
        Serial.println("R:RSSI P:Pulse L:LineMessage C:ContnueLeak N:NoUse");
        PlusTest = 0;
    }
  }
  if (Lf.recieve()) {
    Serial.print(" Rssi from Cordinator=");
    Serial.println(Lf.get_data(0));//文字列を受信
    RssiTest=0;
    digitalWrite(12, HIGH);
  }
  if (!t) {
    Measure();
    SendData('A');
    Monitor();
  }
  if (++t >= TX_INTERVAL) t = 0;//120sec 2min
  delay(922);
}
void SendData(char cmd) {//A:Ambientデータ　L:LINEにメッセージ
  CONNECT();
  Lf.command(cmd);
  Lf.set_data(0, Status);
  Lf.set_data(1, WaterFlow);
  Lf.set_data(2, BattVolt);
  Lf.set_data(3, SolarCur);
  if (Lf.transmit())Lf.setled(LedPin, 1); //データ配列を送信
  delay(1000);
  if (!RssiTest)   digitalWrite(12, HIGH);

}

void Measure() {//2分ごとに呼ばれる
  static int leak;
  long d;
  d = analogRead(1);
  BattVolt = (float)d * 19.80 / 1024; //バッテリ電圧　V
  d = analogRead(0);
  SolarCur = d * 495 / 1024; //太陽電池電流　mA
  WaterFlow = CountWater;
  if (CountWater > 32000)WaterFlow = 32000; //intを超えた時の処理
  CountWater = 0;
  checkFlow();
}
void checkFlow() {//流水を確認してLINEにメッセージを送る
  static int sta0=5;
  Status = 0;
  if (WaterFlow) {
    if (TimLeak < 0)TimLeak = 0;
    else TimLeak++;
    if (TimLeak > TIMLEAK) {
      TimLeak = TIMLEAK;
      Status = 1;
    }
  } else {
    if (TimLeak > 0)TimLeak = 0;
    else TimLeak--;
    if (TimLeak < -TIMNOFLOW) {
      TimLeak = -TIMNOFLOW;
      Status = 2;
    }
  }
  if (sta0 != Status) {
    sta0 = Status;
    SendData('L');
  }
}
void Monitor() {
  Serial.print("Tim(");
  Serial.print(TimLeak);
  Serial.print("),");
  Serial.print(Status);
  Serial.print(",");
  Serial.print(WaterFlow);
  Serial.print(",");
  Serial.print(BattVolt);
  Serial.print(",");
  Serial.println(SolarCur);
}
void IntWaterPlse() {//割り込み処理で水道メータのパイロット回転を計測
  static int tgl = 0;
  CountWater++;
  if (PlusTest == 1) {
    tgl = 1 - tgl;
    digitalWrite(LedPin, tgl);
  }
}
