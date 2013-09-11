#ifndef simpleTimer_h
#define simpleTimer_h
  #if defined(ARDUINO) && ARDUINO > 18
    #include <SPI.h>
  #endif
  
#include <Time.h>

#define NAMES_MAX 12                    // length of device names
#define DEV_MAX 4                       // number of devices
#define EVENTS_MAX 32                   // maximum number of events
#define FIRST_DOW 3                     // use this in code, 3 = Monday is day0, 4 = Sunday is day0
#define NO_DEV DEV_MAX + 1              // ID for no_device

// mode listing
#define MODE_NORMAL 0                   // bits ....00xx
#define MODE_VACATION 1                 // bits ....01xx
#define MODE_AUTOOFF 2                  // bits ....10xx
#define MODE_SPECIAL 3                  // bits ....11xx
// type listing
#define TYPE_NORMAL 0                   // bits ....xx00
#define TYPE_TIMEDRIVEN 1               // bits ....xx01
#define TYPE_COUNTED 2                  // bits ....xx10

#define CHANNEL_MASK 31               // bits xxx11111
#define ADDRESS_SHIFT 5
#define TYPE_MASK 3                   // bits xxxx0011
#define MODE_SHIFT 2


class simpleTimer
{
  public:
    simpleTimer();
    void processEvents();   
    byte getType(byte idx);
    byte getMode(byte idx);
    byte getChannel(byte idx);
    byte getAddress(byte idx);
    void setVacationMode(int days);
    void switchOnOff(byte idx, boolean onoff);
    void setAutoOff(byte idx);
    void switchAll(boolean onoff);
    
    time_t vacEnd;                               // 0 if system operates normally, otherwise end date/time of vacation
    
    struct setup_t {
      byte mac[6];
      byte ip[4];
      byte mask[4];
      byte gw[4];
      byte dnsip[4];
      boolean dhcp;
      char timeZoneOffset;
      unsigned long woeid;
    } stp; 
    
    // devices structure
    struct devices_t {
      boolean status[DEV_MAX];
      char name[DEV_MAX][NAMES_MAX + 1];
      byte adr[DEV_MAX];
      byte type[DEV_MAX];                    // bits 01 = type, 0=normal, 1=time driven, 2=counted; bits 23 = mode, 0=normal, 1=vacation proof, 2=auto off, 3=copied
      time_t count[DEV_MAX];
    } dev; 
    
    struct events_t {
      byte device[EVENTS_MAX];
      time_t t_on[EVENTS_MAX];
      time_t t_off[EVENTS_MAX];
      byte d_on[EVENTS_MAX];
      byte d_off[EVENTS_MAX];
    } events;    
    
  private:
    boolean isTime(byte eidx, byte didx, boolean onoroff);
    
    time_t nw;                                   // global variable for now()
    int nowD;                                    // global variable for weekday(now())
    int nowH;                                    // global variable for hour(now())
    int nowM;                                    // global variable for minute(now())
};  

#endif


