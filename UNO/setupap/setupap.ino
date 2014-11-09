#include <EEPROM.h>
#include <Time.h>
#include "apsaveany.h"

#define NAMES_MAX 12                    // length of device names
#define DEV_MAX 16                      // number of devices
#define EVENTS_MAX 20                   // maximum number of events
#define SPECIAL_MAX 3

// setup structure
struct setup_t {
  byte mac[6];
  byte ip[4];
  byte mask[4];
  byte gw[4];
  byte dnsip[4];
  boolean dhcp;
  char timeZoneOffset;
  unsigned long specCode[SPECIAL_MAX];
}; 

// devices structure
struct devices_t {
  boolean status[DEV_MAX];
  char name[DEV_MAX][NAMES_MAX + 1];
  byte adr[DEV_MAX];
  byte type[DEV_MAX];                    // bits 01 = type, 0=normal, 1=time driven, 2=counted; bits 23 = mode, 0=normal, 1=vacation proof, 2=auto off, 3=copied
  time_t count[DEV_MAX];
}; 

struct events_t {
  byte device[EVENTS_MAX];
  time_t t_on[EVENTS_MAX];
  time_t t_off[EVENTS_MAX];
  byte d_on[EVENTS_MAX];
  byte d_off[EVENTS_MAX];
};

#define STP_START 0
#define DEV_START sizeof(stp)
#define EVENTS_START DEV_START + sizeof(dev)

void setup() {
  setup_t stp = {{0xDE, 0xAD, 0xBE, 0x11, 0xFE, 0xED},        // mac address
                  {192, 168, 1, 60},                          // default ip
                  {255, 255, 255, 0},                         // default netmask
                  {192, 168, 1, 1},                           // default gateway
                  {192, 168, 1, 1},                           // default dns server
                  true,                                       // set true for dhcp
                  0,                                          // timezone
                  {14756260, 14756258, 14756257}
                  };
                  
devices_t dev = {
	{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false},
	{"Switch 1    ", "Switch 2    ", "Switch 3    ", "Switch 4    ", "Switch 5    ", "Switch 6    ", "Switch 7    ", "Switch 8    ", "Switch 9    ", "Switch 10   ", "Switch 11   ", "Switch 12   ", "Switch 13   ", "Switch 14   ", "Switch 15   ", "Switch 16   "},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
events_t events = {
	{16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	};
                   
  Serial.begin(9600);
  Serial.print("Size of the setup structure [bytes]: ");
  Serial.println(sizeof(stp));
  Serial.print("Size of the device structure [bytes]: ");
  Serial.println(sizeof(dev));
  Serial.print("Size of the events structure [bytes]: ");
  Serial.println(sizeof(events));
  Serial.println(STP_START);
  Serial.println(DEV_START);
  Serial.println(EVENTS_START);


  EEPROM_writeAnything(STP_START, stp);
  EEPROM_writeAnything(DEV_START, dev);
  EEPROM_writeAnything(EVENTS_START, events);
  Serial.println("Saved. Please upload Autopower sketch.");
  Serial.end();
}

void loop() {
  
}

