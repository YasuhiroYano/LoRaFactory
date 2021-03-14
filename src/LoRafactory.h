/*
LoRafactory.h
This library is only for Uno like bord TYPE 3276-500.
Made by http://www.kkyes.co.jp/
aa
Private LoRa module EASEL ES920LR https://easel5.com/
V1.03 2021.02.18 KKYES YasuhiroYano

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

#ifndef LoRafactory_h
#define LoRafactory_h

#include <inttypes.h>
#include <Stream.h>
#include <SoftwareSerial.h>

//#include <String.h>

/******************************************************************************
* Definitions
******************************************************************************/

#define _LF_MAX_BUFF 80 // TX,RX buffer size 92>>124
#define _LF_MAX_STRING 50 // Max size of send string
#define _LF_MAX_DATANUM 10 // data size 6>>10
#define _LF_MAX_DATASIZE 12 // each data size 
#define _LF_VERSION 03 // software version of this library

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

class LoRafactory// : public Stream
{
private:
  uint8_t _mask;  //
  uint16_t _rec_io;
  char * _rec_data[_LF_MAX_DATANUM];
  uint8_t _rec_data_num;
  char * _send_data[_LF_MAX_DATANUM];
  uint8_t _send_data_num;
  uint8_t _cordinator;
  uint8_t _sleepmode;
  uint16_t _panid;
  uint16_t _ownid;
  uint16_t _recid;
  int16_t _rssi;
  int8_t _ledptn;

  char _command;
  char _buff[_LF_MAX_BUFF]; //tx,rx buffer
  char _data_str[_LF_MAX_DATANUM][_LF_MAX_DATASIZE];
 
  bool ES_comm(char *cmd,char *ret,float tim);
  bool ES_comm(char *cmd,uint16_t id,char *ret,float tim);
  bool ES_comm(char *cmd,int8_t data,char *ret,float tim);
  void change_io(uint8_t rxio);
  bool rec_header(void);
  public:
  // public methods
  void setled(int ptn);
  void setled(int pin,int ptn);
  void ledpattern(int8_t ptn){_ledptn=ptn;}
  void ledjob(int ledpin);
  void ledjob();
  bool reset();
  void sleepmode(){if(_cordinator==0) _sleepmode=1;}//SleepmodeÇóLå¯Ç…Ç∑ÇÈ
  bool connect(uint16_t dstid);//bw=4(125k) sf=7 default 
  bool connect(uint16_t dstid,int8_t bw,int8_t sf);//Ex bw=3(62.5k) sf=9 default ÇÃ4î{ÇÃí êMéûä‘  
  bool recieve();
  bool transmit();
  bool transmit(char *s);
  uint8_t set_maskBit(uint8_t msk){_mask |= 1<<(msk-2);return _mask;}
  uint8_t reset_maskBit(uint8_t msk){_mask &= ~(1<<(msk-2));return _mask;}
  char * get_data(uint8_t index){return _rec_data[index];}
  int16_t rssi(void){return _rssi;}
  uint16_t recid(void){return _recid;}
  char command(void){return _command;}
  void command(char c){_command=c;}
  void set_data(uint8_t index,char *  data){_send_data[index]=data;}
  void set_data(uint8_t index,long  data);
  void set_data(uint8_t index,int  data);
  void set_data(uint8_t index,uint16_t  data);
  void set_data(uint8_t index,float  data);
  static int library_version() { return _LF_VERSION; }
  LoRafactory(uint8_t cordinator, uint16_t panid,uint16_t ownid,uint8_t receiveNum, uint8_t transmitNum);
  ~LoRafactory();
  
};

// Arduino 0012 workaround
#undef int
#undef char
#undef long
#undef byte
#undef float
#undef abs
#undef round

#endif
