/*  LoRafactory イノシシ罠通報装置　
     LoRaモジュールとマイコンをスリープさせてバッテリ運用
   Ver1.0　2021/03/09 KK.YES 矢野
   Ver1.1  2021/03/13 KK.YES 矢野 帯域幅、拡散係数追加　再送信回数設定

   This library is only for Uno like bord TYPE 3276-500.
   Made by http://www.kkyes.co.jp/
   Private LoRa module EASEL ES920LR https://easel5.com/
   Bord:Arduino Pro or Pro mini ATMega328P (3.3V8MHz)
*/
#include "LoRafactory.h"
#include <avr/sleep.h>                  // スリープライブラリ
#include <avr/wdt.h>                    // ウォッチドッグタイマー ライブラリ

int EndDeviceId = 0x200;
int CordinatorId = 0;
int LoopTime = 450; // 450*8=3600 .. 1時間ごとにライブ送信　電池寿命１年
                    // 1350*8=10800sec ..3時間ごとにすれば２年以上となる
#define RETRYNUM 3//LINE メッセージリトライ回数
int RetryCount=0; //送信成功までリトライ回数


LoRafactory Lf(0, 1, EndDeviceId, 2, 2); //End device
//(cordinator,panid,ownid,receiveNum,transmitNum);
int SwOutPin = 2; //リードスイッチを読み取るための出力
int SwInPin = 3; //リードスイッチを読み取るための入力
int Status;//状態LINEにメッセージ番号　0:正常　3:罠動作検知


void setup() {
  Serial.begin(115200);
  Lf.sleepmode();
  if (Lf.connect(CordinatorId, 3, 10))Lf.setled(3); //送信先を指定して接続
  pinMode(SwOutPin, OUTPUT);
  pinMode(SwInPin, INPUT_PULLUP);
  digitalWrite(SwOutPin, HIGH);
  Serial.println("This is for a Boar Trup.");
  setWdt();
}

void loop() {
  static int t = LoopTime;
  static int sta0;
  static int i;
  Status = checksw() ? 4 : 3;//LINEメッセージ番号　3:正常　4:罠動作検知
  wdt_reset();
  if (sta0 != Status) {
    RetryCount = RETRYNUM;
    sta0 = Status;
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
  Lf.command(cmd);
  Lf.set_data(0, Status);
  Lf.set_data(1, i++);
  if (Lf.transmit()) {
//    Lf.setled(1);
    if(cmd=='L') RetryCount = 0;
  } else Lf.setled(0);
}
int8_t checksw(void) { //接点OFFで１
  int8_t sw;
  digitalWrite(SwOutPin, LOW);
  sw = digitalRead(SwInPin);
  digitalWrite(SwOutPin, HIGH);
  return sw;
}
void setWdt() {
  // ADCを停止（消費電流 147→27μA）
  ADCSRA &= ~(1 << ADEN);
  MCUSR = 0;
  WDTCSR |= 0b00011000; //WDCE WDE set
  WDTCSR =  0b01000000 | 0b100001;//WDIE set  |WDIF set  scale 8 seconds
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // パワーダウンモード指定
  sleep_enable();
}
//----------------------------------------
ISR(WDT_vect) {                         // WDTがタイムアップした時に実行される処理
}
