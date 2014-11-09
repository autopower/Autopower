#include <EEPROM.h>
#include <Time.h>
#include "apsaveany.h"

#define NAMES_MAX 12                    // length of device names
#define DEV_MAX 16                      // number of devices
#define EVENTS_MAX 20                   // maximum number of events
#define SPECIAL_MAX 3                   // maximum number of special codes 

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

#define STP_START 0
#define DEV_START sizeof(stp)
#define EVENTS_START DEV_START + sizeof(dev)

// no-cost stream operator as described at 
// http://sundial.org/arduino/?page_id=119
template<class T>
inline Print &operator <<(Print &obj, T arg)
{ obj.print(arg); return obj; }

void setup() {
byte i;

  EEPROM_readAnything(STP_START, stp);
  EEPROM_readAnything(DEV_START, dev);
  EEPROM_readAnything(EVENTS_START, events);
                     
  Serial.begin(9600);
  Serial.print("Size of the setup structure [bytes]: ");
  Serial.println(sizeof(stp));
  Serial.print("Size of the device structure [bytes]: ");
  Serial.println(sizeof(dev));
  Serial.print("Size of the events structure [bytes]: ");
  Serial.println(sizeof(events));
  Serial << "Total: " << sizeof(stp) + sizeof(dev) + sizeof(events) << "\n";

  Serial << "setup_t stp = {\n";
  Serial << "\t{" << stp.mac[0] << ", " << stp.mac[1] << ", " << stp.mac[2] << ", " << stp.mac[3] << ", " << stp.mac[4] << ", " << stp.mac[5] << "},\n";
  Serial << "\t{" << stp.ip[0] << "," << stp.ip[1] << ", " << stp.ip[2] << ", " << stp.ip[3] << "},\n";
  Serial << "\t{" << stp.mask[0] << "," << stp.mask[1] << ", " << stp.mask[2] << ", " << stp.mask[3] << "},\n";
  Serial << "\t{" << stp.gw[0] << "," << stp.gw[1] << ", " << stp.gw[2] << ", " << stp.gw[3] << "},\n";
  Serial << "\t{" << stp.dnsip[0] << "," << stp.dnsip[1] << ", " << stp.dnsip[2] << ", " << stp.dnsip[3] << "},\n";
  Serial << "\t" << (stp.dhcp?"true":"false") << ",\n";
  Serial << "\t" << byte(stp.timeZoneOffset) <<"\n";
  Serial << "\t" << "{";
  for (i = 0; i < SPECIAL_MAX; i++) {
    Serial << stp.specCode[i];
    if (i < SPECIAL_MAX - 1) Serial << ", ";
  }
  Serial << "}\n";
  Serial << "\t};\n";
  
  Serial << "devices_t dev = {\n\t{";
  for (i = 0; i < DEV_MAX; i++) {
    Serial << (dev.status[i]?"true":"false");
    if (i < DEV_MAX - 1) Serial << ", ";
  }
  Serial << "},\n";
  
  Serial << "\t{";
  for (i = 0; i < DEV_MAX; i++) {
    Serial << "\"" << dev.name[i] << "\"";
    if (i < DEV_MAX - 1) Serial << ", ";
  }
  Serial << "},\n";
  
  Serial << "\t{";
  for (i = 0; i < DEV_MAX; i++) {
    Serial << dev.adr[i];
    if (i < DEV_MAX - 1) Serial << ", ";
  }
  Serial << "},\n";
  
  Serial << "\t{";
  for (i = 0; i < DEV_MAX; i++) {
    Serial << dev.type[i];
    if (i < DEV_MAX - 1) Serial << ", ";
  }
  Serial << "},\n";
  
  Serial << "\t{";
  for (i = 0; i < DEV_MAX; i++) {
    Serial << dev.count[i];
    if (i < DEV_MAX - 1) Serial << ", ";
  }
  Serial << "}\n\t};\n";

// EVENTS
  Serial << "events_t events = {\n\t{";
  for (i = 0; i < EVENTS_MAX; i++) {
    Serial << events.device[i];
    if (i < EVENTS_MAX - 1) Serial << ", ";
  }
  Serial << "},\n";
  
  Serial << "\t{";
  for (i = 0; i < EVENTS_MAX; i++) {
    Serial << events.t_on[i];
    if (i < EVENTS_MAX - 1) Serial << ", ";
  }
  Serial << "},\n";

  Serial << "\t{";
  for (i = 0; i < EVENTS_MAX; i++) {
    Serial << events.t_off[i];
    if (i < EVENTS_MAX - 1) Serial << ", ";
  }
  Serial << "},\n";

  Serial << "\t{";
  for (i = 0; i < EVENTS_MAX; i++) {
    Serial << events.d_on[i];
    if (i < EVENTS_MAX - 1) Serial << ", ";
  }
  Serial << "},\n";

  Serial << "\t{";
  for (i = 0; i < EVENTS_MAX; i++) {
    Serial << events.d_off[i];
    if (i < EVENTS_MAX - 1) Serial << ", ";
  }
  Serial << "},\n\t};";
  
  Serial.end();
}

void loop() {
  
}

