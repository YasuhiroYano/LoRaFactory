/*  LoRafactory イノシシ罠通報装置　
     LoRaモジュールとマイコンをスリープさせてバッテリ運用
   Ver1.0　2021/03/09 KK.YES 矢野
   Ver1.1  2021/03/13 KK.YES 矢野 帯域幅、拡散係数追加　再送信回数設定
   Ver1.2  2021/05/02 KK.YES 矢野　バッテリ電圧測定追加 電源起動時の送信停止
   This library is only for Uno like bord TYPE 3276-500.
   Made by http://www.kkyes.co.jp/
   Private LoRa module EASEL ES920LR https://easel5.com/
   Bord:Arduino Pro or Pro mini ATMega328P (3.3V8MHz)
*/
#include "LoRafactory.h"
#include <avr/sleep.h>                  // スリープライブラリ
#include <avr/wdt.h>                    // ウォッチドッグタイマー ライブラリ

int MyID = 0x202;   //0x7fff以下で設定する事
int DestId = 0x300; //中継器使用　直接Gatewayに送るときは　0
int LoopTime = 450; // 450*8=3600 .. 1時間ごとにライブ送信　電池寿命１年
// 1350*8=10800sec ..3時間ごとにすれば２年以上となる
#define RETRYNUM 3  //LINE メッセージリトライ回数
int RetryCount = 0; //送信成功までリトライ回数


LoRafactory Lf(0, 10, MyID, 2, 3); //End device
//(cordinator,panid,ownid,receiveNum,transmitNum);
int SwOutPin = 2;     //リードスイッチを読み取るための出力
int SwInPin = 3;      //リードスイッチを読み取るための入力
int BattCheckPin = 4; //バッテリ電圧を計測するための出力
#define KV 6.71       //バッテリ電圧計測用係数　1.1*61/10

int Status;           //状態LINEにメッセージ番号　3:正常　4:罠動作検知
int StatusLast;       //前回状態
#define  CONNETCT()  Lf.connect(DestId, 3, 10)  ////bw=3(62.5k) sf=10

void setup() {
  Serial.begin(115200);
  Lf.sleepmode();
  if (CONNETCT())Lf.setled(3); //送信先を指定して接続
  pinMode(SwOutPin, OUTPUT);
  pinMode(SwInPin, INPUT_PULLUP);
  pinMode(BattCheckPin, OUTPUT);
  digitalWrite(SwOutPin, HIGH);
  digitalWrite(BattCheckPin, LOW);
  Serial.println("\n\rThis is for a Boar Trup v1.2");
  analogReference(INTERNAL);
  setWdt();
  digitalWrite(9, HIGH);//debug
  delay(1000);
  StatusLast = checksw() ? 4 : 3;
}

void loop() {
  static int t = 0;
  static int i;
  Status = checksw() ? 4 : 3;//LINEメッセージ番号　3:正常　4:罠動作検知
  wdt_reset();
  if (StatusLast != Status) {
    RetryCount = RETRYNUM;
    StatusLast = Status;
  }
  if (RetryCount) {
    RetryCount--;
    SendData('L');//状態が変化したらLINEにメッセージを送信
  }
  if (++t >= LoopTime) {
    t = 0;
    SendData('T');//AmbientにRSSIを送信する
  }
  sleep_mode();//８秒間スリープする
}
void SendData(char cmd) {//T:Ambient RSSI データ　L:LINEにメッセージ
  static int i;
  float batt=BattVolt();
  Lf.command(cmd);
  Lf.set_data(0, Status);
  Lf.set_data(1, i++);
  Lf.set_data(2, batt);
  if (Lf.transmit()) {
    Serial.print(batt);
    Serial.print(" ");
    Serial.println(i);
    if (cmd == 'L') RetryCount = 0;
  } else {
    Serial.println("error");
    Lf.setled(0);
  }
  delay(1000);
}
int8_t checksw(void) { //接点OFFで１
  int8_t sw;
  digitalWrite(SwOutPin, LOW);
  sw = digitalRead(SwInPin);
  digitalWrite(SwOutPin, HIGH);
  return sw;
}
float BattVolt() { //バッテリ電圧計測
  float a0;
  ADCSRA |= (1 << ADEN);
  digitalWrite(BattCheckPin, HIGH);
  a0 = analogRead(0);
  delay(1);
  a0 = analogRead(0);
  a0 = a0 * KV / 1024 - 0.07;
  digitalWrite(BattCheckPin, LOW);
  ADCSRA &= ~(1 << ADEN);// ADCを停止
  return a0;
}
void setWdt() {
  ADCSRA &= ~(1 << ADEN); // ADCを停止（消費電流 147→27μA）
  MCUSR = 0;
  WDTCSR |= 0b00011000; //WDCE WDE set
  WDTCSR =  0b01000000 | 0b100001;//WDIE set  |WDIF set  scale 8 seconds
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // パワーダウンモード指定
  sleep_enable();
}

//----------------------------------------
ISR(WDT_vect) {                         // WDTがタイムアップした時に実行される処理
}
