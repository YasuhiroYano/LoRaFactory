/*
LoRafactory.h
This library is only for Uno like board TYPE 3276-500.
Made by http://www.kkyes.co.jp/
Private LoRa module EASEL ES920LR https://easel5.com/
v1.02 2021.03.02  

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

The latest version of this library can always be found at
http://arduiniana.org.
*/
// 
// Includes
// 
#include <avr/pgmspace.h>
#include "Arduino.h"
#include "pins_arduino.h"
#include "LoRafactory.h"
//#include "icrmacros.h"
#define LEDPIN 13
#define ES_RESET 12
#define ES_TX 11
#define ES_RX 10
#define ES_SLEEP 9

//
// Statics
//
//SoftwareSerial EsSerial;
SoftwareSerial EsSerial(ES_RX, ES_TX);
//
// Constructor
//
LoRafactory::LoRafactory(uint8_t cordinator, uint16_t panid,uint16_t ownid,uint8_t receiveNum, uint8_t transmitNum) 
{
    if (receiveNum>_LF_MAX_DATANUM) receiveNum=_LF_MAX_DATANUM;
    if (transmitNum>_LF_MAX_DATANUM) transmitNum=_LF_MAX_DATANUM;
    _rec_data_num=receiveNum;
    _send_data_num=transmitNum;
    _cordinator= cordinator;
    _panid = panid;
    _ownid = ownid;
    _command=' ';
    pinMode(ES_RESET, OUTPUT);
    digitalWrite(ES_RESET, HIGH);
    pinMode(ES_SLEEP, OUTPUT);
    digitalWrite(ES_SLEEP, LOW);
    pinMode(LEDPIN, OUTPUT);
    digitalWrite(LEDPIN, HIGH);
    EsSerial.begin(9600);
}

//
// Destructor
//
LoRafactory::~LoRafactory()
{
}

#define hex(c)  (((c) >9)? (c) -10+'a': (c) +'0')
void ctoh(char *s, uint8_t c){
	uint8_t d;
	d=c>>4;
	s[0]=hex(d);
	d=c & 0xf;
	s[1]=hex(d);
	s[2]=0;
}
void utoh(char *s, uint16_t d)
{
	ctoh(s,d>>8);
	ctoh(s+2,d & 0xff);
}
int16_t inhex(uint8_t c){ //convert hex char to int
	if ((c -= '0') <= 9 || 10 <= (c -= 'A'-'0'-10) && c <= 15)
		return (c);
	return (-1);
}


uint16_t htoi(char *p){   //convert hex string to int
	int16_t c;
	uint16_t v=0;
	for (;  (c = inhex(*p)) != -1;  p++)
		v = v * 16 + c;
	return (v);
}

bool LoRafactory::ES_comm(char *cmd,char *ret,float tim){
    uint32_t t=0;
    uint32_t tmax=tim*1000;
    uint8_t p=0;
    char c;
    if(strlen(cmd)){
        EsSerial.println(cmd);
 //        Serial.println(cmd);
    }
    while(t<tmax){
        delay(1);
        t++;
        while(EsSerial.available()){
            c=EsSerial.read();
            _buff[p++]=c;
            if(p >= _LF_MAX_BUFF) p = _LF_MAX_BUFF-1;
            if(c=='\r' ){
                _buff[p]=0;
                if(strstr(_buff,ret)) return true; 
                p=0;//flush when another responce 
   //             Serial.println(_buff);
            }
        }
    }
    return false;//time out
}
bool LoRafactory::ES_comm(char *cmd,uint16_t id,char *ret,float tim){
    char s[10];
    utoh(s,id);
    strcpy(_buff,cmd); strcat(_buff,s);  
    return ES_comm(_buff,ret,tim);
}

void LoRafactory::change_io(uint8_t rxio){
    uint16_t mask=_mask;
    uint16_t io = PORTB;
    io = (io<<6)+(PORTD>>2);
    io &= ~mask;
    io |= mask & rxio;
    PORTD = io << 2;
    PORTB = io >> 6;
}

//
// Public methods
//

bool LoRafactory::reset(){
     //reset
    digitalWrite(ES_RESET, HIGH);
    delay(1000); 
    digitalWrite(ES_RESET, LOW);
    delay(10); 
    return true;
}
  
  
bool LoRafactory::connect(uint16_t dstid){
    reset();
    ES_comm("","Select",3);
    ES_comm("2","OK",2);
    if(_cordinator)   ES_comm("node 1","OK",1);
    else ES_comm("node 2","OK",1);
    ES_comm("ownid ",_ownid,"OK",1);
    ES_comm("rcvid 1","OK",1);
    ES_comm("rssi 1","OK",1);
    ES_comm("dstid ",dstid,"OK",1);
    if(_sleepmode) ES_comm("sleep 3","OK",1);
    return ES_comm("start","OK",1);
 }
void LoRafactory::set_data(uint8_t index,long  data){
    sprintf(_data_str[index],"%ld",data);
    _send_data[index]=_data_str[index];
}
void LoRafactory::set_data(uint8_t index,int  data){
    sprintf(_data_str[index],"%d",data);
    _send_data[index]=_data_str[index];
}
void LoRafactory::set_data(uint8_t index,uint16_t  data){
    sprintf(_data_str[index],"%d",data);
    _send_data[index]=_data_str[index];
}
void LoRafactory::set_data(uint8_t index,float  data){
    dtostrf(data, 3, 2, _data_str[index]);
    _send_data[index]=_data_str[index];
}

bool LoRafactory::transmit(char *s){
    _send_data[0]=s;
    _send_data_num=1;
    transmit();
}

bool LoRafactory::transmit(){
    bool rtn = true;
    int8_t i;
    uint8_t io;
    char s[_LF_MAX_STRING];
    digitalWrite(ES_SLEEP, LOW);
    io=PIND>>2;
    io+=PINB<<6;
    ctoh(_buff,io);
    _buff[2]=_command;
    _buff[3]=0;
    for(i=0;i<_send_data_num;i++){
        if(_send_data[i]==NULL) return false;
        strcat(_buff,",");
        strcat(_buff,_send_data[i]);
    }
    if(!ES_comm(_buff,"OK",5)) rtn = false;
    if(_sleepmode) digitalWrite(ES_SLEEP, LOW);   
    return rtn;
 }

 bool LoRafactory::rec_header(void){
    char s[10];
    if(!ES_comm("","",0.05))  return false; 
    if(strlen(_buff)<14) return false;//not recieved data 
    strncpy(s,_buff,4);
    _rssi=htoi(s);
    strncpy(s,&_buff[8],4);
    _recid=htoi(s);
    strncpy(s,&_buff[12],2);
    s[2]=0;
    change_io(htoi(s));
    _command=_buff[14];
    return true;
 }
bool LoRafactory::recieve(){
    int8_t i;
    char *p;
    if(!rec_header()) return false;
    p=strtok(_buff,",");
    for(i=0;i<_LF_MAX_DATANUM;i++){
       p=strtok(NULL,",");
       _rec_data[i]=p;
       if(p==NULL) break;
    }
    return true;
}

void LoRafactory::setled(int ptn) {
    setled(LEDPIN,ptn);
}
void LoRafactory::setled(int ledpin,int ptn) {
  static int i;
  if (ptn == -1) {  //ƒgƒOƒ‹
    i = i ? 0 : 1;
    digitalWrite(ledpin, i);
    return;
  }
  if (ptn == 0) { //
    digitalWrite(ledpin, LOW);
    delay(20);
    digitalWrite(ledpin, HIGH);
    return;
  }
  digitalWrite(ledpin, LOW);
  delay(1000);
  digitalWrite(ledpin, HIGH);
  for (i = 0; i < ptn; i++) {
    delay(300);
    digitalWrite(ledpin, LOW);
    delay(200);
    digitalWrite(ledpin, HIGH);
  }
  delay(1000);
}
void LoRafactory::ledjob(){
    ledjob(LEDPIN);
}
void LoRafactory::ledjob(int ledpin){//10msec‚ÌŠ„‚èž‚Ý‚ÅŒÄ‚Ño‚·‚æ‚¤‚É
    static int8_t sta=0;
    static int8_t ptn=0;
    static int8_t t=0;
    switch(sta){
        case 0:
            ptn=_ledptn;
            if(_ledptn == -1) sta=1;
            else if(_ledptn == 0) sta=2;
            else if(_ledptn > 0)sta=10;
            break;
        case 1:
            digitalWrite(ledpin,1- digitalRead(ledpin));
            sta=100;
            break;
        case 2:
            digitalWrite(ledpin,LOW);
            sta=100;
            break;
        case 10:
            digitalWrite(ledpin,LOW);
            t=100;
            sta++;
            break;
        case 11:
            if(--t)break;
            digitalWrite(ledpin,HIGH);
            t=30;
            sta++;
            break;
        case 12:
            if(--t)break;
            digitalWrite(ledpin,LOW);
            t=30;
            sta++;
            break;
        case 13:
            if(--t)break;
            digitalWrite(ledpin,HIGH);
            if(--_ledptn > 0){
               t=30;
               sta=12;
            }  else sta=100;
            break;
        case 100:            
            digitalWrite(ledpin,HIGH);
            _ledptn=-2;
            sta=0;
            break;
        default:
            sta=0;            
    }
}