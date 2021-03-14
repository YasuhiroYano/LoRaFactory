/*  LoRafactory WiFi Gateway　
   Ver1.0　2021/01/15 KK.YES 矢野
   Ver1.1　2021/02/07 KK.YES 矢野
   Ver1.2 2021/03/13 KK.YES 矢野 帯域幅、拡散係数変更に伴う変更
   This library is only for Uno like bord TYPE 3276-500.
   Made by http://www.kkyes.co.jp/
   Private LoRa module EASEL ES920LR https://easel5.com/
   Bord:Arduino Pro or Pro mini ATMega328P (3.3V8MHz)
   WiFi module ESP-WROOM-02
*/
//WiFi の接続情報
const char* ssid = "YESGUEST";//利用するWiFiの合わせて変更
const char* password = "YESGUEST";//利用するWiFiの合わせて変更
#define SCHDULE 8 //定刻通信時刻

//Ambient の接続情報　構造体に　Enddevice　のidとともに登録する
struct EnddeviceInfo {
  unsigned int lf_id;//LoLoRafactory　Enddevice　id
  const char* ambientChannelId;//Ambient　チャンネル番号
  const char* ambientwriteKey;
  const char* lineNotifyToken;
};
struct EnddeviceInfo inftbl[] = {
  { 0x100, "31852", "5e047e050a8a4047",
    "GvlMd5Wzmgbuop9E1JGjSOnFmiP4m1xt65O7szYXnxh"
  },//漏水監視１　
  { 0x200, "33443", "26e1a1847e0a4b74",
    "GvlMd5Wzmgbuop9E1JGjSOnFmiP4m1xt65O7szYXnxh"
  },//イノシシ罠１　テストのために漏水監視と同じにしている　要変更　
  //Enddevice（子機）が増えたときにここに追加登録する
  {0, 0, "", ""} //最後のデータはlf_id=0とする
};
#define INFNUM (sizeof(inftbl)/sizeof(EnddeviceInfo))
int8_t Sendflg[INFNUM];
char *LineMsg[] = {
  " 定時通報 異常なし。 https://ambidata.io/bd/board.html?id=21973# ",
  " 漏水の恐れがあります。 https://ambidata.io/bd/board.html?id=21973# ",
  " 水道が使用されていません。 https://ambidata.io/bd/board.html?id=21973# ",
  " イノシシ罠　定時通報 異常なし。 ",
  " !!イノシシ捕獲!!　確認に来てください",
  " 本日の来訪者人数　約",
  " パンフレットが無くなりました。補充してください。",
  ""
};
#define MSGNUM (sizeof(LineMsg)/sizeof(char *))
#include "LoRafactory.h"
LoRafactory Lf(1, 10, 0, 2, 2);  //cordinator
//(cordinator,panid,ownid,receiveNum,transmitNum);
uint16_t Destid = 0x10;
#define CONNECT() Lf.connect(Destid,3,10) //bw=3(62.5k) sf=10

#define MAX_BUF 80
char buf[MAX_BUF];//受信文字数　最大80


int Ledpin = 7;
void setup() {
  delay(1000);
  Serial.begin(57600);//115200bauではWROOM-02からのデータを取れない　
  pinMode(Ledpin, OUTPUT);
  Lf.setled(Ledpin, 1);
  Serial.println(" LoRafactory WiFi Gateway v1.00 PAN=10");
  //デバック情報としてシリアルに出力するときは1文字目はスペースにする。(重要)
  if (CONNECT()) {
    Serial.println(" LoRa Connected");
    Lf.setled(Ledpin, 2); //送信先を指定して接続
  }
  if (WifiConnect()) Lf.setled(Ledpin, 3); //WiFiに接続 max20sec
}

void loop() {
  int inf;
  if (Lf.recieve()) {//データ配列を受信
    inf = lookupid(Lf.recid());
    if (inf == -1) {
      Serial.print(" !Unknown Enddevice rec id=");
      Serial.println(Lf.recid());
      return;
    }
    Serial.print(" rec id=");
    Serial.print(Lf.recid());
    Serial.print(" RSSI=");
    Serial.println(Lf.rssi());
    switch (Lf.command()) {
      case 'R'://受信強度を送り返す
        Rssi();
        break;
      case 'A'://WROOM-02に対してAmbientにデータを送るよう要求する
        AmbientSend(inf);
        if (!Sendflg[inf]) {//定時通報　LINEにメッセージを送る
          LineSend(inf);
          Sendflg[inf] = 1;
        }
        break;
      case 'T'://WROOM-02に対してAmbientにRSSIを送るよう要求する
        AmbientSendRssi(inf);
        if (!Sendflg[inf]) {//定時通報　LINEにメッセージを送る
          LineSend(inf);
          Sendflg[inf] = 1;
        }
        break;
      case 'L'://WROOM-02に対してLINEにメッセージを送るよう要求する
        LineSend(inf);
        break;
    }
    Lf.setled(Ledpin, 1);
    Lf.setled(1);
  }
  if (rec()) { //WROOM-02から時刻を受信する
    buf[0] = ' '; //先頭を空白にすることでコマンドと解釈されない
    if (buf[4] == ':') {
      buf[4] = 0;
      clearTimeFlg(&buf[2]);//時間文字列
    }
  }
}
void clearTimeFlg(char *s) { //定時になったら送信フラグをクリアする
  static int hour0;
  int i;
  int hr = atoi(s);
  if (hr != hour0 && hr == SCHDULE) {
    for (i = 0; i < INFNUM; i++)      Sendflg[i] = 0;
    hour0 = hr;
  }
}
bool rec(char *s, unsigned long t) {
  while (t) {
    if (rec() && strstr(buf, s)) return true;
    --t;
    delay(1);
  }
  return false;
}
bool rec() {
  char c;
  static int recpos = 0;
  while (Serial.available()) {
    c = Serial.read();
    Serial.print(c);//デバッグ用にシリアルに送る
    buf[recpos] = c;
    if (c == '\n') {
      if (recpos > 0)recpos--;
      buf[recpos] = '\0';//手前のCRをクリア
      recpos = 0;
      return true;
    }   else {
      if (recpos <= MAX_BUF - 1) recpos++;
    }
  }
  return false;
}
void Rssi() {
  Serial.println("RSSI");
  if ( Destid != Lf.recid()) { //接続先が異なる場合はリセットして再接続する
    Destid = Lf.recid();
    if (CONNECT()) {
      Lf.setled(Ledpin, 2); //送信先を指定して接続
      Serial.print(" Reconnect to ");
      Serial.println(Destid);
    }
  }
  Lf.set_data(0, Lf.rssi());
  Lf.set_data(1, 0); //拡張用
  Lf.transmit();
}
void AmbientSend(int inf) {
  AmbientConnect(inf);
  Serial.print("S");
  Serial.print(Lf.get_data(0));
  Serial.print(",");
  Serial.print(Lf.get_data(1));
  Serial.print(",");
  Serial.print(Lf.get_data(2));
  Serial.print(",");
  Serial.println(Lf.get_data(3));
}
void AmbientSendRssi(int inf) {
  AmbientConnect(inf);
  Serial.print("S");
  Serial.print(Lf.get_data(0));
  Serial.print(",");
  Serial.print(Lf.get_data(1));
  Serial.print(",");
  Serial.println(Lf.rssi());
}
bool WifiConnect() {
  Serial.print("W");
  Serial.print(ssid);
  Serial.print(",");
  Serial.println(password);
  return rec("IP address", 60000);
}
bool AmbientConnect(int inf) {
  Serial.print("A");
  Serial.print(inftbl[inf].ambientChannelId);
  Serial.print(",");
  Serial.println(inftbl[inf].ambientwriteKey);
  return true;
}

void LineSend(int inf) {
  int msgno = 0;
  msgno = atoi( Lf.get_data(0));
  if (msgno < 0 || msgno >= MSGNUM) {
    Serial.print(" Error invalid message number ");
    Serial.println(msgno);
    return;
  }
  Serial.print("L");
  Serial.print(inftbl[inf].lineNotifyToken);
  Serial.print(", id=");
  Serial.print(Lf.recid());
  Serial.println(LineMsg[msgno]);
}
int lookupid(int lf_id) {
  int i;
  for (i = 0; i < 20; i++) {
    if (inftbl[i].lf_id == 0) return -1;
    if (inftbl[i].lf_id == lf_id)break;
  }
  return i;
}
