/*  LoRafactory WiFi Gateway　
   Ver1.0　2021/01/15 KK.YES 矢野
   Ver1.1　2021/02/07 KK.YES 矢野
   Ver1.2 2021/03/13 KK.YES 矢野 帯域幅、拡散係数変更に伴う変更
   Ver1.3 2021/04/27 KK.YES 明石 中継器からの中継データ受信対応
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
    "3cNGx99ndIchqOPicymQfK9F70JiwIIN1AlsfbjLLmj"
  },//イノシシ罠　　
  
  { 0x300, "34372", "7f52f0571be732bd",
    "3cNGx99ndIchqOPicymQfK9F70JiwIIN1AlsfbjLLmj"
  },//イノシシ中継罠　　
  
  //Enddevice（子機）が増えたときにここに追加登録する
  {0, 0, "", ""} //最後のデータはlf_id=0とする
};
#define INFNUM (sizeof(inftbl)/sizeof(EnddeviceInfo))
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

#define ENDDEVICENUM 10  //中継器を含まない子機数が10を超えるときに変更
int LineSendflg[ENDDEVICENUM][2];  //中継器から中継されてきた子機一覧を保持する     

int Ledpin = 7;
void setup() {
  int i;
  
  delay(1000);
  Serial.begin(57600);//115200bauではWROOM-02からのデータを取れない　
  pinMode(Ledpin, OUTPUT);
  Lf.setled(Ledpin, 1);
  Serial.println(" LoRafactory WiFi Gateway v1.3 PAN=10");
  //デバック情報としてシリアルに出力するときは1文字目はスペースにする。(重要)
  if (CONNECT()) {
    Serial.println(" LoRa Connected");
    Lf.setled(Ledpin, 2); //送信先を指定して接続
  }
  if (WifiConnect()) Lf.setled(Ledpin, 3); //WiFiに接続 max20sec
}

void loop() {
  int inf;
  int deviceid = 0;
  int arraypos = 0;

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
    Serial.print(Lf.rssi());
    Serial.print(" inf=");
    Serial.println(inf);
    switch (Lf.command()) {
      case 'T'://WROOM-02に対してAmbientにRSSIを送るよう要求する
        AmbientSendRssi(inf);
        if(checkDailySend(Lf.recid())){
            LineSend(inf,Lf.recid());          
        }
        break;
      case 'P': //中継器経由でｽﾃｰﾀｽ送信
        AmbientRepeater(inf);
        deviceid = atoi( Lf.get_data(3));
        if(checkDailySend(deviceid)){
            LineSend(inf,deviceid);          
        }
        break;
      case 'C': //中継器経由でLINE送信
        deviceid = atoi( Lf.get_data(3));
        LineSend(inf,deviceid);
        break;
      case 'L'://WROOM-02に対してLINEにメッセージを送るよう要求する
        LineSend(inf,Lf.recid());
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
  int i,j;
  int hr = atoi(s);
  if (hr != hour0 && hr == SCHDULE){
    for(i = 0;i < ENDDEVICENUM; i++){
          LineSendflg[i][1] = 0;
    }      
  }
  hour0 = hr;
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
void AmbientRepeater(int inf) { //中継器経由での子機情報をAmbientへ送信
  AmbientConnect(inf);
  Serial.print("S");
  Serial.print(Lf.get_data(0)); //ｽﾃｰﾀｽ
  Serial.print(",");
  Serial.print(Lf.get_data(1)); //ｲﾝｸﾘﾒﾝﾄ
  Serial.print(",");
  Serial.print(Lf.get_data(2)); //子機電圧
  Serial.print(",");
  Serial.print(Lf.get_data(3)); //ID
  Serial.print(",");
  Serial.print(Lf.get_data(4)); //中継器電圧
  Serial.print(",");
  Serial.print(Lf.get_data(5)); //中継器電流
  Serial.print(",");
  Serial.println(Lf.get_data(6)); //子機電波強度
}

void AmbientSendRssi(int inf) { //子機情報をAmbientへ送信
  AmbientConnect(inf);
  Serial.print("S");
  Serial.print(Lf.get_data(0)); //ｽﾃｰﾀｽ
  Serial.print(",");
  Serial.print(Lf.get_data(1)); //ｲﾝｸﾘﾒﾝﾄ
  Serial.print(",");
  Serial.print(Lf.get_data(2)); //子機電圧
  Serial.print(",");
  Serial.println(Lf.rssi());    //子機電波強度
}
bool WifiConnect() {  //Wifi接続
  Serial.print("W");
  Serial.print(ssid);
  Serial.print(",");
  Serial.println(password);
  return rec("IP address", 60000);
}
bool AmbientConnect(int inf) {  //Ambient接続
  Serial.print("A");
  Serial.print(inftbl[inf].ambientChannelId);
  Serial.print(",");
  Serial.println(inftbl[inf].ambientwriteKey);
  return true;
}
void LineSend(int inf,int id) { //LINE 送信処理
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
  Serial.print(id);
  Serial.println(LineMsg[msgno]);
}

int lookupid(int lf_id) { //line idの検索
  int i;
  for (i = 0; i < INFNUM; i++) {
    if (inftbl[i].lf_id == 0) return -1;
    if (inftbl[i].lf_id == lf_id)break;
  }
  return i;
}

bool checkDailySend(int id){ //中継された子機がLINEの1日1回通報を実行済が確認して未送信ならtrue 送信済みならfalseを返す
  int i;
  for(i=0;i<ENDDEVICENUM;i++){
    if(LineSendflg[i][0]==0){ //登録されていないIDなら値を設定して送信する
      LineSendflg[i][0] = id;
      LineSendflg[i][1] = 1;
      return true;
    }else if(LineSendflg[i][0]==id){  //idが存在して未送信なら送信する
      if(LineSendflg[i][1]==0){
        LineSendflg[i][1] = 1;
        return true;
      }else{
        return false;
      }
    }
  }
  
  return false;  
}
