#define TRACE_MEM
#define DEBUG_PRG

#define USE_ARDUINO_SPI_LIBRARY 1

#if defined(ARDUINO) && ARDUINO > 18
  #include <SPI.h>
#endif

#include <itoa.h>
#include <Time.h> 
#include <SdFat.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#define WEBDUINO_FAVICON_DATA ""
#define WEBDUINO_FAIL_MESSAGE ""
#define WEBDUINO_AUTH_MESSAGE ""
#define WEBDUINO_SERVER_ERROR_MESSAGE ""
#define WEBDUINO_AUTH_REALM "AutoPower"
#include <WebServer.h>
#include <RemoteReceiver.h>
#include <RemoteTransmitter.h>

#ifdef TRACE_MEM
  #include <malloc.h>
  #include <stdlib.h>
  #include <stdio.h>
  extern char _end;
  extern "C" char *sbrk(int i);
  char *ramstart=(char *)0x20070000;
  char *ramend=(char *)0x20088000;
#endif

// ----------------------- DEFINE VALUES -----------------------
#define VERSION_STRING "1.00"           // version, not used anywhere
#define NAMES_MAX 12                    // length of device names
#define DEV_MAX 16                      // number of devices
#define EVENTS_MAX 20                   // maximum number of events
#define SPECIAL_MAX 3                   // maximum number of special codes
#define NAMELEN 15                      // maximum length of POST name
#define VALUELEN 15                     // maximum length of POST value
#define ntpSyncTime SECS_PER_HOUR * 4   // how often sync time, in seconds
#define NTP_PACKET_SIZE 48              // NTP time stamp is in the first 48 bytes of the message
#define localPort 8888                  // local port to listen for UDP packets
#define FAILED 2                        // command num of failure msg
#define UNAUTH 3                        // command num of unathorized msg
#define SWITCH_POS 0                    // where is the switching menu
#define SETUP_POS 1                     // where is the setup menu
#define MAX_EXTEND 3                    // how many hour you can extend switch off time, in hours
#define CON_BUFFER 48                   // size of connection buffer for webserver.processconnection
#define DIVIDER 2                       // how many switches is on one row?
#define FIRST_DOW 3                     // use this in code, 3 = Monday is day0, 4 = Sunday is day0
#define REMOTE_SIGNAL_COUNT 3           // how many times must be signal from remote controller received
#define NO_DEV DEV_MAX + 1              // ID for no_device
#define TRANSMITTER_PIN 7               // where's the transmitter?, pin 7
#define ETH_PIN 10                      // where is the SS for ethernet
#define SD_PIN 4                        // where is the SS for SD card


// mode listing
#define MODE_NORMAL 0                   // bits ....00xx
#define MODE_VACATION 1                 // bits ....01xx
#define MODE_AUTOOFF 2                  // bits ....10xx
#define MODE_SPECIAL 3                  // bits ....11xx
// type listing
#define TYPE_NORMAL 0                   // bits ....xx00
#define TYPE_TIMEDRIVEN 1               // bits ....xx01
#define TYPE_COUNTED 2                  // bits ....xx10

// ----------------------- VARIABLES -----------------------
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

// ----------------------- NTP variables -----------------------         
// IPAddress timeServer(158, 195, 19, 4);  // sk.pool.ntp.org
const IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov 
// IPAddress timeServer(132, 163, 4, 102);    // time-b.timefreq.bldrdoc.gov
EthernetUDP Udp;                           // definition of udp packet
byte packetBuffer[NTP_PACKET_SIZE];        // buffer to hold incoming and outgoing NTP packets 

// ----------------------- web server values and inits -----------------------
WebServer webserver("", 80);
char name[NAMELEN];
char value[VALUELEN];
const char adm[21] = "YWRtaW46YWRtaW4=";     // in other words: "YWRtaW46YWRtaW4=" is the Base64 representation of "admin:admin" 
const char usr[21] = "dXNlcjp1c2Vy";         // "dXNlcjp1c2Vy" is the Base64 representation of "user:user"
const char fChar[5] = " #!S";

// time variables  
time_t vacEnd = 0;                           // 0 if system operates normally, otherwise end date/time of vacation
time_t nw;                                   // global variable for now()
int nowD;                                    // global variable for weekday(now())
int nowH;                                    // global variable for hour(now())
int nowM;                                    // global variable for minute(now())
const char shortDay[] = "MoTuWeThFrSaSu";    // weekday short names
static char timeBuffer[6] = "00:00";         // time buffer for formatting time
static byte offDay[EVENTS_MAX], onDay[EVENTS_MAX];

// RF 433MHZ switches values
ActionTransmitter actionTX(TRANSMITTER_PIN);

// Other
boolean setupSwitch = true;                  // in times of setup() this is set to true
SdFat sd;                                    // SD card
const char config_file[] = "config.ini";     // config filename
const char config_file_bak[] = "config.bak"; // backup config filename

// ----------------------- HTML CODE -----------------------
P(menuFooter) = "</tr></tbody></table>\n";
P(htmlEnd) = "</body></html>";
P(tableStart0) = "<table border=\"0\" cellpadding=\"";
P(tableStart1) = "2\" cellspacing=\"2";
P(tableStart2) = "20\" cellspacing=\"4";
P(tableStart3) = "\">\n<tbody>\n";
P(tableEnd) = "</tbody>\n</table>\n";
P(tableHead1) = "<thead><tr><th>Name</th><th>Chn</th><th>Addr</th><th>Nm/Tm/Cn</th><th>Nm/Vc/AO/Sp</th></thead>";
P(tableHead2) = "<thead><tr><th>Dev ID</th><th>Time on</th><th>Days on</th><th>Time off</th><th>Days off</th></tr></thead>";
P(trS) = "\n<tr>";
P(trE) = "</tr>\n";
P(tdS) = "\n\t<td>";
P(tdE) = "</td>";
P(input2) = "\" value=\"";
P(selectS) = "<select name=\"";
P(optionS) = "\n\t\t<option";
P(optionV) = " value=\"";
P(close1)  = "\">";
P(input3) =  "\"/>";
P(optionE) = "</option>";
P(selectE) = "</select>";
P(inputChk) = "\n\t\t<input type=\"checkbox\" name=\"";                               // name
P(inputRadio) = "\n\t\t<input type=\"radio\" name=\"";                                // switch type 0=classic, 1=timed, 2=counter
P(postS) = "<form action=\"/";
P(postE) = "\" method=\"post\">";
P(postSettings) = "setuppost";
P(adminSettings) = "adminpost";
P(submit) = "<input type=\"submit\" value=\"Save\"/></form>";
P(checked) = "\" checked/>";
P(selected) = " selected";
P(refresh) = "<meta http-equiv=\"refresh\" content=\"0; URL=";
P(mtxt0) = "Status";
P(mtxt1) = "Setup";
P(menuswitch) = "swt.html";
P(endslash) = "\"";

#ifdef DEBUG_PRG
  unsigned long debug_val_1 = 0;// here add declaration of variables you need
  unsigned long debug_val_2 = 0;
  
void printDebug() {
  webserver.print(F("<p>"));
  webserver.print(debug_val_1);
  webserver.print("<br/>");
  webserver.print(debug_val_2);
  webserver.print(F("</p>"));
}
#endif

#ifdef TRACE_MEM
void printMemory() {
char *heapend=sbrk(0);
register char * stack_ptr asm ("sp");
struct mallinfo mi=mallinfo();

  webserver.print(F("<p>"));
  webserver.print(F("DRam:"));
  webserver.println(mi.uordblks);
  webserver.print(F("<br/>PStatic:"));
  webserver.println(&_end - ramstart);
  webserver.print(F("<br/>Stack:"));
  webserver.println(ramend - stack_ptr); 
  webserver.print(F("<br/>Free:"));
  webserver.println(stack_ptr - heapend + mi.fordblks);
  webserver.print(F("</p>"));
}
#endif


// -------------------------------------------------------
// ----------------------- helpers -----------------------
// -------------------------------------------------------
#define CHANNEL_MASK 31               // bits xxx11111
#define ADDRESS_SHIFT 5
#define TYPE_MASK 3                   // bits xxxx0011
#define MODE_SHIFT 2

byte getChannel(byte idx) {
  return (dev.adr[idx] & CHANNEL_MASK);
}

byte getAddress(byte idx) {
  return (dev.adr[idx] >> ADDRESS_SHIFT); 
}

byte getType(byte idx) {
  return (dev.type[idx] & TYPE_MASK);
}

byte getMode(byte idx) {
  return (dev.type[idx] >> MODE_SHIFT);
}

void getTime() {
  nw = now();
  // return DOW in order, according to FIRST_DOW (Monday=0..Sunday=6)
  nowD = ((nw / 86400L) + FIRST_DOW) % 7;
  nowH = hour(nw);
  nowM = minute(nw);
}

time_t convertTime() {
time_t temp = 0;
char timeStr[3] = "00";
  timeStr[0] = value[0];
  timeStr[1] = value[1];
  temp = atol(timeStr) * SECS_PER_HOUR;
  timeStr[0] = value[3];
  timeStr[1] = value[4];  
  temp = temp + (atol(timeStr) * SECS_PER_MIN);
  return temp;
}

void setVacationMode(int days) {
  if (days > 0) vacEnd += now() + (days * SECS_PER_DAY);
    else vacEnd = 0;
}

char *getName(char a, byte n) {
static char tempStr[4] = "abc";

  tempStr[0] = a;
  if (n < 10) {
    tempStr[1] = '0';
    tempStr[2] = char(48 + n);
    } else {
      tempStr[1] = char(48 + (n / 10));
      tempStr[2] = char(48 + (n % 10)); 
  }
  return tempStr;
}

// -------------------------------------------------------
// --------------------- HTML Helpers --------------------
// -------------------------------------------------------
void printTextBox(const bool pwd, const byte txtSize, const char *txtname, const char *txtvalue) {
  P(inp01) = "<input type=\"";
  P(impmxl) = "\" maxlenth=\"";
  P(impsize) = "\" size=\"";
  P(impname) = "\" name=\"";
  P(password) = "password";
  P(text) = "text";
  P(value) = " value=\"";
  P(endtag) = "/>";

  webserver.printP(inp01);
  if (pwd) webserver.printP(password);
    else webserver.printP(text);
  webserver.printP(impmxl);
  webserver.print(txtSize);
  webserver.printP(impsize);
  webserver.print(txtSize);
  webserver.printP(impname);
  webserver.print(txtname);
  webserver.printP(endslash);
  if (!pwd) {
    webserver.printP(value);
    webserver.print(txtvalue);
    webserver.printP(endslash);
  }
  webserver.printP(endtag);
}


// print time @ which is on or off
void printTime(byte idx, byte code) {
time_t tempTime;
  webserver.printP(tdS);
  if (code == 0) tempTime = events.t_off[idx];
   else tempTime = events.t_on[idx]; 
  sprintf(timeBuffer, "%02d:%02d", hour(tempTime), minute(tempTime));
  if (code == 0) printTextBox(false, 5, getName('t', idx), timeBuffer);
    else printTextBox(false, 5, getName('T', idx), timeBuffer);
  webserver.printP(tdE); 
}

// print days @ which is on or off
void printDays(byte idx, byte code) {
byte value;

  if (code == 0) value = events.d_off[idx];
    else value = events.d_on[idx];
    
  webserver.printP(tdS);
  for (int j=0; j<7; j++) {  
    webserver.printP(inputChk);
    if (code == 0) webserver.print(getName('d', idx));
      else webserver.print(getName('D', idx));
    webserver.print(j);
    webserver.printP(input2);
    webserver.print(j);
    if (bitRead(value, j) == 1) webserver.printP(checked);
      else webserver.printP(input3);
  }
  webserver.printP(tdE);    
}



// --------------------------------------------------------------
// ----------------------- HTML functions -----------------------
// --------------------------------------------------------------
void printPageStart(byte type) {
byte shortDayPos = 0;

  P(htm01) = "<html>\n<head><title>Autopower</title></head>\n<body>\n";
  P(menu1) = "<ul id=\"nav\">";
  P(menu2) = "</ul>";
  P(style0) = "<style>\nbody {font-weight:bold;font-family:sans-serif;text-decoration:none;list-style:none;}\nh1 {background:#069;padding:2em 1em 1em 1em;color:white;border-radius:10px 10px;}\nh2 {background:#0AA;padding:0.5em;color:white;border-radius:5px;}";
  P(style1) = " {width:100%;float:left;margin:0;padding:0;background-color:#f2f2f2;border-bottom:1px solid #ccc;border-top:1px solid #ccc;}";
  P(navli) = "\n#nav li ";
  P(style2) = "\n#nav li {float:left;}";
  P(style3) = "\n#nav li a {display:block;padding:8px 15px;border-right:1px solid #ccc;color:#069;text-decoration:none;}";
  P(style4) = "\n#nav li a:hover {color:#c00;background-color:#fff;}";
  P(style5) = " {display:block;min-width:8em;padding:1em 2em 1em 2em;margin:0.5em;text-decoration:none;color:#066;border-right:1px solid #ccc;border-left: 1em solid #";
  P(nav) = "\n#nav";
  P(swdivtf) = " {border-left: 1em solid ";
  P(swdivt1) = "#0f0;}";
  P(swdivf1) = "#f00;}";
  P(swdivt0) = "\n#sw div.t";
  P(swdivf0) = "\n#sw div.f";
  P(styleend) = "\n</style>\n";
  P(swdiv) = "\n#sw div {float:left;margin:0.5em;text-decoration:none;color:#066;display:block}";
  P(swdivswitch) = "\n#sw div.switch {min-width:9em;padding:1em 2em 1em 1em;border-right:1px solid #ccc;}";
  P(swadd) = "\n#sw div.add0 {border-right:0px;min-width:7em;padding:1em 0em 1em 1em;}\n#sw div.add1 {border-right:1px solid #ccc;min-width:1em;padding:1em 1em 1em 1em;}";
  P(liahref) = "<li><a href=\"";
  P(ali) = "</a></li>";
  P(vn0) = "vacation";
  P(vn1) = "normal";
  P(nh1) = "\n<h1>";
  P(switchall_0) = "?cmd=all";
  P(switchall_1) = "all ";
  P(switchon) = "on";
  P(switchoff) = "off";
  P(days2) = "?vac=2\">";
  P(clock1) = "?clk=1\">";
  P(nsw) = "\n#sw";
  P(h1e) = "</h1>";
  P(menuitem0) = "<li><a href=\"swt.html\">Status</a></li><li><a href=\"stp.html\">Setup</a></li>";
  P(menuitem9) = "<li><a href=\"https://www.facebook.com/autopow\">About</a></li>";
  P(li_s) = "<li>";
  P(li_e) = "</li>";
  P(failed) = "404 Not found!";
  P(unauth) = "401 Unauthorized!";
  
  
  switch (type) {
    case UNAUTH: 
      webserver.httpUnauthorized();
      break;
    case FAILED:
      webserver.httpFail();
      break;
    default:
      webserver.httpSuccess();
  }
    
  webserver.printP(htm01);
  // <style>\nbody {font-weight:bold;font-family:sans-serif;text-decoration:none;list-style:none;}\nh1 {background:#069;padding:2em 1em 1em 1em;color:white;border-radius:10px 10px;}
  webserver.printP(style0);
  
  // \n#nav {width:100%;float:left;margin:0;padding:0;background-color:#f2f2f2;border-bottom:1px solid #ccc;border-top:1px solid #ccc;}
  webserver.printP(nav);
  webserver.printP(style1);
  
  // \n#nav li {float:left;};
  webserver.printP(style2);
  
  // \n#nav li a {display:block;padding:8px 15px;border-right:1px solid #ccc;color:#069;text-decoration:none;}
  webserver.printP(style3);
  
  // \n#nav li a:hover {color:#c00;background-color:#fff;}
  webserver.printP(style4);
  
  // \n#sw
  //  {width:100%;float:left;margin:0;padding:0;background-color:#f2f2f2;border-bottom:1px solid #ccc;border-top:1px solid #ccc;}
  // \n#sw div {float:left;margin:0.5em;text-decoration:none;color:#066;display:block}
  webserver.printP(nsw);
  webserver.printP(style1);
  webserver.printP(swdiv);

  // \n#sw div.switch {min-width:9em;padding:1em 2em 1em 1em;border-right:1px solid #ccc;}
  // \n#sw div.t
  //  {border-left: 1em solid 
  // #0f0;}
  // \n#sw div.f
  //  {border-left: 1em solid 
  // #f00;}
  webserver.printP(swdivswitch);
  webserver.printP(swdivt0);
  webserver.printP(swdivtf);
  webserver.printP(swdivt1);
  webserver.printP(swdivf0);
  webserver.printP(swdivtf);
  webserver.printP(swdivf1);
  
  // \n#sw div.add0 {border-right:0px;min-width:7em;padding:1em 0em 1em 1em;}\n#sw div.add1 {border-right:1px solid #ccc;min-width:1em;padding:1em 1em 1em 1em;}
  // \n</style>\n
  // <ul id=\"nav\">
  webserver.printP(swadd);
  webserver.printP(styleend);
  webserver.printP(menu1);
  
  // <li><a href=\"swt.html\">Status</a></li><li><a href=\"setup.html\">Setup</a></li>
  webserver.printP(menuitem0);
  
  webserver.printP(liahref);
  webserver.printP(menuswitch);
  webserver.printP(clock1);
  shortDayPos = nowD << 1;
  webserver.print(shortDay[shortDayPos]);
  webserver.print(shortDay[shortDayPos + 1]);
  webserver.print(", ");
  sprintf(timeBuffer, "%02d:%02d", hour(now()), minute(now()));
  webserver.print(timeBuffer);
  webserver.printP(ali);
  
  webserver.printP(liahref);
  webserver.printP(menuswitch);
  webserver.printP(days2);
  if (vacEnd > 0) webserver.printP(vn0);
    else webserver.printP(vn1);
  webserver.printP(ali);
  
  // print about us
  webserver.printP(menuitem9);
  
  webserver.printP(menu2);
  webserver.printP(nh1);
  switch (type) {
    case UNAUTH: 
      webserver.printP(unauth);
      break;
    case FAILED:
      webserver.printP(failed);
      break;
    default:
      if (type == 0) webserver.printP(mtxt0);
       else webserver.printP(mtxt1);
  }
  webserver.printP(h1e);
#ifdef TRACE_MEM || DEBUG_PRG
  webserver.print(F("<h2>"));
#endif

#ifdef TRACE_MEM
  printMemory();
#endif

#ifdef DEBUG_PRG
  printDebug();
#endif

#ifdef TRACE_MEM || DEBUG_PRG
  webserver.print(F("</h2>"));
#endif

}

void printSwitches(boolean switching) {
P(a1) = " onClick=\"document.location.href='swt.html?";                     // if you change swt.html you MUST change link here !!!
P(a20) = "cmd=";
P(a21) = "add=";
P(a3) = "';\" style=\"cursor:pointer;\">";
P(divsw) = "\n<div id=sw>\n";
P(divswon) = "\t<div class=\"switch t";
P(divswoff) = "\t<div class=\"switch f";
P(divsadd) = " add0\"";
P(divsadd10) = "\t<div class=\"add1\"";
P(divsadd11) = "+";
P(divE) = "</div>\n";
byte special;

    webserver.printP(divsw);
    for (byte n = 0; n < DEV_MAX; n++){
      special = 0;
      if (getType(n) == TYPE_TIMEDRIVEN) special = 1;
        else if (getMode(n) != MODE_NORMAL) special = 2;

      if (dev.status[n] == true) webserver.printP(divswon);
        else webserver.printP(divswoff);
      if (special > 0) webserver.printP(divsadd);
        else webserver.printP(endslash);
      if (switching) {
        webserver.printP(a1);
        webserver.printP(a20);
        webserver.print(n);
        webserver.printP(a3);
      } else webserver.print(">");

      webserver.print(dev.name[n]);

      if (special > 0) {
        webserver.printP(divE);
        webserver.printP(divsadd10);
     
        if (special == 1) {
          webserver.printP(a1);
          webserver.printP(a21);
          webserver.print(n);
          webserver.printP(a3);
          if (dev.count[n] > 0) webserver.print(dev.count[n]);
            else webserver.printP(divsadd11);
        } else {
          webserver.print('>');
          webserver.print(fChar[getMode(n)]);
        }
      
      } 
      webserver.printP(divE);
      
      if ((n + 1) % DIVIDER == 0){
        webserver.printP(divE);
        webserver.printP(divsw);
      }
    }
 
  webserver.printP(divE);
}

void switchesCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
URLPARAM_RESULT rc; 
int a;
 
  if (server.checkCredentials(usr) || server.checkCredentials(adm)) {
    printPageStart(SWITCH_POS); 
    if (type != WebServer::HEAD) {
      if (strlen(url_tail)) {
        while (strlen(url_tail)) {
          rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
          if (rc != URLPARAM_EOS) {
            a = atoi(value);
            if (strcmp(name, "cmd") == 0) {
              switchOnOff(a, !dev.status[a]); 
              setAutoOff(a);
            }         
            if (strcmp(name, "vac") == 0) setVacationMode(a);
            if (strcmp(name, "clk") == 0) setTime(getTimeAndDate());
            if (strcmp(name, "add") == 0) {
              dev.count[a]++;
              if (dev.count[a] > MAX_EXTEND) dev.count[a] = 0;
            }
          }
        }
        doRefresh();
      } 
    
    printSwitches(true); 
    server.printP(htmlEnd);
    } 
  } else { 
    server.httpUnauthorized();
    printPageStart(SWITCH_POS); 
    printSwitches(false);
  }
}

void radio(char name, byte idx) {
byte test, max_value;
char *nameStr;

  if (name == 'm') max_value = 4;
    else max_value = 3;
  nameStr = getName(name, idx);
  webserver.printP(tdS);
  for (byte j = 0; j < max_value; j++) {
    webserver.printP(inputRadio);
    webserver.print(nameStr);
    webserver.printP(input2);
    webserver.print(j);
    if (name == 'p') test = getType(idx);
      else test = getMode(idx);
    if (test == j) webserver.printP(checked);
       else webserver.printP(input3);
  }
  webserver.printP(tdE);
}

void dropDown(char name, byte idx) {
byte values, test;
P(noDevice) = "n/a (0)";

  webserver.printP(tdS);
  webserver.printP(selectS);
  webserver.print(getName(name, idx));
  webserver.printP(close1);
  switch (name) {
    case 'c': 
        values = 32;
        break;
    case 'a': 
        values = 5;
        break;
    case 'e': 
        values = DEV_MAX + 1;
  }
  for (byte j = 0; j < values; j++) {
    webserver.printP(optionS);
    switch (name) {
      case 'c':
        test = getChannel(idx);
        break;
      case 'a':
        test = getAddress(idx);
        break;
      case 'e':
        test = events.device[idx];
    }
    if (test == j) webserver.printP(selected);
    
    webserver.printP(optionV);
    webserver.print(j);
    webserver.printP(close1);
    switch (name) {
      case 'a':
        webserver.print(char(j + 65));
        break;
      case 'c':
        webserver.print(j);
        break;
      case 'e':
        if (j == DEV_MAX) webserver.printP(noDevice);
          else {
            webserver.print(dev.name[j]);
            webserver.print(" (");
            webserver.print(j+1);
            webserver.print(")"); 
          }
        break;  
    }
    webserver.printP(optionE);
  }
  webserver.printP(selectE);
  webserver.printP(tdE);
}

void printTableStart(byte startType) {
P(devHead) = "<h2>Devices</h2>";
P(eventsHead) = "<h2>Events</h2>";
  if (startType == 1) webserver.printP(devHead);
    else webserver.printP(eventsHead);
  webserver.printP(tableStart0);
  webserver.printP(tableStart1);
  webserver.printP(tableStart3);
  if (startType == 1) webserver.printP(tableHead1);  
    else webserver.printP(tableHead2);
}

void setupCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
P(txt1) = "Vacation [days]:";
P(tzo) = "Time zone [hour]:";
P(spec) = "Spec code";
P(otherHead) = "<h2>Other</h2>";
P(br) = "<br/>";
char tmpstr[10];
int tmpint = 0;
int i, j;
 
  if (server.checkCredentials(adm)) {
    printPageStart(SETUP_POS);
    if (type != WebServer::HEAD)  {
      server.printP(postS);
      server.printP(postSettings);
      server.printP(postE);
      printTableStart(1);
     
      for (i = 0; i < DEV_MAX; i++) {
        // row start
        server.printP(trS);          
        // name 'n'
        server.printP(tdS);
        printTextBox(false, NAMES_MAX, getName('n', i), dev.name[i]);
        server.printP(tdE);          
        // channel 'c'
        dropDown('c', i);
        // address 'a'
        dropDown('a', i);    
        // type 'p'
        radio('p', i);
        // mode 'm'
        radio('m', i);
        server.printP(trE);   
      }
      server.printP(tableEnd);
      
      printTableStart(2);
      
      for (i = 0; i < EVENTS_MAX; i++) {
        // row start
        server.printP(trS);
        // dev ID
        dropDown('e', i);      
        // timed start & days
        printTime(i, 1);
        printDays(i, 1);
        // timed end & days
        printTime(i, 0);
        printDays(i, 0);
        // row end
        server.printP(trE);
      }
      server.printP(tableEnd);    
    
      // new block
      server.printP(otherHead);
      for (i = 0; i < SPECIAL_MAX; i++) {
        server.printP(spec);
        server.print(i);
        ltoa(stp.specCode[i], tmpstr, 10);
        printTextBox(false, 8, getName('s', i), tmpstr);
        server.printP(br);  
      }
      
      server.printP(txt1);
      if (vacEnd > 0) tmpint = (vacEnd - now()) / SECS_PER_DAY;
      itoa(tmpint, tmpstr, 10);
      printTextBox(false, 3, "v00", tmpstr);
      server.printP(br);
      server.printP(tzo);
      itoa(stp.timeZoneOffset, tmpstr, 10);
      printTextBox(false, 3, "z00", tmpstr);
      server.printP(br);
      server.printP(submit);
      server.printP(htmlEnd);
    }
  }
  else printPageStart(UNAUTH);
} 

void processSetupParam() {
byte i, j, idx;
long v;
char index[3];

  index[0] = name[1];
  index[1] = name[2];
  idx = atoi(index);
  if (name[0] != 'n') v = atol(value);
  switch (name[0]) {
    case 'n':
      for (j = 0; j < NAMES_MAX; j++) {
        if (j < strlen(value)) dev.name[idx][j] = value[j];
          else dev.name[idx][j] = ' ';
      }
      break;
    case 'c':
      dev.adr[idx] = (getAddress(idx) << ADDRESS_SHIFT) + v;
      break;
    case 'a':
      dev.adr[idx] = getChannel(idx) + (v << ADDRESS_SHIFT);
      break;
    case 'p':
      dev.type[idx] = (getMode(idx) << MODE_SHIFT) + v;
      break;
    case 'm':
      dev.type[idx] = getType(idx) + (v << MODE_SHIFT);
      break;
    case 'e':
      events.device[idx] = v;
      break;
    case 'T':
      events.t_on[idx] = convertTime();
      break;
    case 't':
      events.t_off[idx] = convertTime();
      break;
    case 'D':
      bitSet(onDay[idx], v);
      break;
    case 'd':
      bitSet(offDay[idx], v);
      break;
    case 'z':
      stp.timeZoneOffset = v;
      if (!setupSwitch) getTimeAndDate();
      break;
    case 'v':
      setVacationMode(v);
      break;
    case 's':
      stp.specCode[idx] = v;
      break;
  }
}

void setupPost(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
byte i, j, idx;
long v;
char index[3];
boolean repeat;

  if(type == WebServer::POST) {
    for (i = 0; i < EVENTS_MAX; i++) {
      onDay[i] = 0;
      offDay[i] = 0;
    }
    do {
      repeat = server.readPOSTparam(name, NAMELEN, value, VALUELEN);
      processSetupParam();
    } while (repeat);
    
    for (i = 0; i < EVENTS_MAX; i++) {
      events.d_on[i] = onDay[i];
      events.d_off[i] = offDay[i];
    }
    printPageStart(SETUP_POS);
    saveSetup();
    // setup saved do refresh
    doRefresh();
  }
}

void doRefresh() {
  webserver.printP(refresh);
  webserver.printP(menuswitch);
  webserver.printP(close1);
}

void failureCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
  printPageStart(FAILED);  
  server.printP(htmlEnd);
}


// -------------------------------------------------------
// -------------------- Device helpers -------------------
// -------------------------------------------------------

// plain send
#define PULSE_LENGTH 350
#define REPEAT_TRANSMIT 10

void transmit(int nHighPulses, int nLowPulses) {
  digitalWrite(TRANSMITTER_PIN, HIGH);
  delayMicroseconds(PULSE_LENGTH * nHighPulses);
  digitalWrite(TRANSMITTER_PIN, LOW);
  delayMicroseconds(PULSE_LENGTH * nLowPulses);
}

void send01(unsigned long code) {
  for (byte n = 0; n < REPEAT_TRANSMIT; n++) {
    for (byte i = 0; i < 24; i++) {
      if (bitRead(code, 23 - i) == 1) transmit(3, 1);
        else transmit(1, 3);
    }
    // send sync
    transmit(1, 31);
  }
}

// check for automatically switched off outlet, if so set variable. Outlet cannot be time driven or counted
void setAutoOff(byte idx) {
  // its simple, must be type 0 and mode autooff (2), so 2bits=00, 2bits=10
  for (byte i = 0; i  < EVENTS_MAX; i++) {
    if (events.device[i] == idx && getMode(idx) == MODE_AUTOOFF) {              // if you find first events with this outlet and this outlet is AutoOff setup values and exit loop
      dev.count[idx] = now() + (hour(events.t_off[i]) * SECS_PER_HOUR) + (minute(events.t_off[i]) * SECS_PER_MIN);
      break;
    }
  }
}

// receive command from original remote
void receiveCommand(unsigned long receivedCode, unsigned int period) {
byte tmpCh = 0;
byte tmpAdr = 0;
byte i;
unsigned long y;
boolean rcvCmd = false;

  // decode tribits
  for (i=0; i<12; i++) {
    y = receivedCode % 3;
    receivedCode = receivedCode / 3;
    
    if (i == 0 && y == 0) rcvCmd = true;
    if (i >= 2 && i <= 6 && y == 0) tmpAdr = (6 - i);
    if (i >= 7 && y == 1) bitSet(tmpCh, i - 7);
  }

  for (i = 0; i < DEV_MAX; i++) {
    if (getChannel(i) == tmpCh && getAddress(i) == tmpAdr) {
      dev.status[i] = rcvCmd;
      if (rcvCmd == true) setAutoOff(i);
    }
  }
}

// switch device on or off
void switchOnOff(byte idx, boolean onoff) {
  dev.status[idx] = onoff;
  // disable receiving, not to interfere with actual command
  RemoteReceiver::disable();

  if (getMode(idx) == MODE_SPECIAL) send01(stp.specCode[getChannel(idx)]);
    else actionTX.sendSignal(getChannel(idx), char(65 + getAddress(idx)), onoff);

  // enable receiving
  RemoteReceiver::enable();
}

#ifdef SWITCH_ALL
void switchAll(boolean onoff) {
  for (byte i = 0; i < DEV_MAX; i++) switchOnOff(i, onoff);
}
#endif

// is this that time we can switch on or off?
boolean isTime(byte eidx, byte didx, boolean onoroff) {
  int swOffDay = nowD;
  int swOffHour = hour(events.t_off[eidx]);
  
  if (onoroff == false) {
      if (swOffHour + dev.count[didx] > 23) {
        if (nowD + 1 > 6) swOffDay = 0;
          else swOffDay = nowD + 1;
        swOffHour = (swOffHour + dev.count[didx]) - 24;
      } else swOffHour = swOffHour + dev.count[didx];
      if (bitRead(events.d_off[eidx], swOffDay) == 1 && swOffHour == nowH && minute(events.t_off[eidx]) == nowM && dev.status[didx] == true) return true;
    } else if (bitRead(events.d_on[eidx], nowD) == 1 && hour(events.t_on[eidx]) == nowH && minute(events.t_on[eidx]) == nowM && dev.status[didx] == false) return true;
  return false;
}

void processEvents() {
int cM, nM, oM;
byte mode, dIdx;

  getTime();
  if (nw >= vacEnd) vacEnd = 0;
  for (int i = 0; i < EVENTS_MAX; i++) {
    dIdx = events.device[i]; 
    if (dIdx == NO_DEV) continue;                  // if device index in events are bigger then device list size, then this event isn't processible
    mode = getMode(dIdx);

    if (vacEnd == 0 || mode == MODE_VACATION) {
      switch (getType(dIdx)) {
        case TYPE_NORMAL:
          if (mode == MODE_AUTOOFF && nw >= dev.count[dIdx] && dev.status[dIdx] == true) switchOnOff(dIdx, false);
          break;
        case TYPE_TIMEDRIVEN:
          if (isTime(i, dIdx, true) == true) switchOnOff(dIdx, true);
          if (isTime(i, dIdx, false) == true) switchOnOff(dIdx, false);
          break;
        case TYPE_COUNTED:
          nM = nowH * 60 + nowM;
          if (dev.status[i]) {
            cM = hour(dev.count[dIdx]) * 60 + minute(dev.count[dIdx]);
            oM = hour(events.t_off[i]) * 60 +  minute(events.t_off[i]);
            if (nM > cM + oM) switchOnOff(dIdx, false);
          } else {
              cM = hour(events.t_on[i]) * 60 + minute(events.t_on[i]);
              if (nM % cM == 0) {
                 switchOnOff(dIdx, true);
                 dev.count[dIdx] = nw;
              }
          } // end else
      } // end switch
    } // end if
  } // end for
}


// -------------------------------------------------------
// -------------------- Setup helpers --------------------
// -------------------------------------------------------
void loadDefaultSetup() {
  setup_t temp_stp = {{0xDE, 0xAD, 0xBE, 0x11, 0xFE, 0xED},          // mac address
                  {192, 168, 1, 60},                          // default ip
                  {255, 255, 255, 0},                         // default netmask
                  {192, 168, 1, 1},                           // default gateway
                  {192, 168, 1, 1},                           // default dns server
                  true,                                       // set true for dhcp
                  0,                                          // timezone
                  {14756260, 14756258, 14756257}
                  };                
  devices_t temp_dev = {
	{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false},
	{"Switch 1    ", "Switch 2    ", "Switch 3    ", "Switch 4    ", "Switch 5    ", "Switch 6    ", "Switch 7    ", "Switch 8    ", "Switch 9    ", "Switch 10   ", "Switch 11   ", "Switch 12   ", "Switch 13   ", "Switch 14   ", "Switch 15   ", "Switch 16   "},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};
  events_t temp_events = {
	{16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	};

  stp = temp_stp;
  dev = temp_dev;
  events = temp_events;
}


bool readPOSTparam(SdFile &df, char *name, int nameLen, char *value, int valueLen) {
char ch;

  memset(name, 0, nameLen);
  memset(value, 0, valueLen);
  --nameLen;
  --valueLen;
  while (df.available()) {
    ch = df.read();
    switch (ch) {
      case '+':
        ch = ' ';
        break;
      case '=':
        nameLen = 0;
        continue;
      case '&':
        return true;
        break;
      case '%':
        char ch1 = df.read();
        char ch2 = df.read();
        if (ch1 == -1 || ch2 == -1) return false;
        char hex[3] = { ch1, ch2, 0 };
        ch = strtoul(hex, NULL, 16);
        break;
    }
    if (nameLen > 0) {
      *name++ = ch;
      --nameLen;
    } else if (valueLen > 0) {
      *value++ = ch;
      --valueLen;
    }
  }
  return false;
}

void loadSetup() {
  SdFile dataFile;
  if (dataFile.open(config_file), O_READ) {
    if (dataFile.fileSize() > 8192) {
      return;
    }
    for (byte i = 0; i < EVENTS_MAX; i++) {
      onDay[i] = 0;
      offDay[i] = 0;
    }
    while (readPOSTparam(dataFile, name, NAMELEN, value, VALUELEN)) {
      processSetupParam();
    }
    for (byte i = 0; i < EVENTS_MAX; i++) {
      events.d_on[i] = onDay[i];
      events.d_off[i] = offDay[i];
    }
  }
  dataFile.close();
}

void saveSetup() {
SdFile dataFile;
byte i, j;
time_t tempTime;

  if (sd.exists(config_file)) {
    sd.remove(config_file_bak);
  }
  if (sd.exists(config_file)) {
    sd.rename(config_file, config_file_bak);
  }
  if (dataFile.open(config_file, O_TRUNC | O_WRITE | O_CREAT)) {
    for (i = 0; i < DEV_MAX; i++) {
      dataFile.print(getName('n', i));
      dataFile.print("=");
      for (j = 0; j < NAMES_MAX; j++) {
        if (dev.name[i][j] == ' ') dataFile.print('+');
          else dataFile.print(dev.name[i][j]);
      }
      dataFile.print("&");
      
      dataFile.print(getName('c', i));
      dataFile.print("=");
      dataFile.print(getChannel(i));
      dataFile.print("&");
      
      dataFile.print(getName('a', i));
      dataFile.print("=");
      dataFile.print(getAddress(i));
      dataFile.print("&");

      dataFile.print(getName('p', i));
      dataFile.print("=");
      dataFile.print(getType(i));
      dataFile.print("&");
      
      dataFile.print(getName('m', i));
      dataFile.print("=");
      dataFile.print(getMode(i));
      dataFile.print("&");
    }
    
    for (i = 0; i < EVENTS_MAX; i++) {
      dataFile.print(getName('e', i));
      dataFile.print("=");
      dataFile.print(events.device[i]);
      dataFile.print("&");
      
      tempTime = events.t_on[i];
      sprintf(timeBuffer, "%02d:%02d", hour(tempTime), minute(tempTime));
      dataFile.print(getName('T', i));
      dataFile.print("=");
      dataFile.print(timeBuffer);
      dataFile.print("&");
   
      Serial.println(events.d_on[i]);   
      for (j = 0; j < 7; j++) {  
        if (bitRead(events.d_on[i], j) == 1) {
          Serial.print("D");
          Serial.print(i);
          Serial.print(".");
          Serial.print(j);
          Serial.print("=");
          Serial.println(j);
          dataFile.print(getName('D', i));
          dataFile.print(j);
          dataFile.print("=");
          dataFile.print(j);
          dataFile.print("&");
        }
      }

      tempTime = events.t_off[i]; 
      sprintf(timeBuffer, "%02d:%02d", hour(tempTime), minute(tempTime));
      dataFile.print(getName('t', i));
      dataFile.print("=");
      dataFile.print(timeBuffer);
      dataFile.print("&");

      for (j = 0; j < 7; j++) {  
        if (bitRead(events.d_off[i], j) == 1) {
          dataFile.print(getName('d', i));
          dataFile.print(j);
          dataFile.print("=");
          dataFile.print(j);
          dataFile.print("&");
        }
      }
    }
  
    for (i = 0; i < SPECIAL_MAX; i++) {
      dataFile.print(getName('s', i));
      dataFile.print("=");
      dataFile.print(stp.specCode[i]);
      dataFile.print("&");
    }
    
   dataFile.print("v00=");
   if (vacEnd > now()) dataFile.print((vacEnd - now()) / SECS_PER_DAY);
     else dataFile.print('0');
   dataFile.print("&");
   dataFile.print("z00=");
   dataFile.print(stp.timeZoneOffset);
   dataFile.print("&END=1");
  }  
  dataFile.close();
}



// -----------------------------------------------------
// ----------------------- SETUP -----------------------
// -----------------------------------------------------

void setup() {
  #ifdef DEBUG_PRG
    Serial.begin(9600);
    Serial.println("GO!");
  #endif

  loadDefaultSetup();
  // read setup
  #ifdef DEBUG_PRG
    Serial.println("SD");
  #endif

 if (!sd.begin(SD_PIN, SPI_HALF_SPEED)) sd.initErrorHalt();
 
  #ifdef DEBUG_PRG
    Serial.println("SETUP LOAD");
  #endif
  loadSetup();
  
  #ifdef DEBUG_PRG
    Serial.println("DHCP");
  #endif
  // start network DHCP or not
  if (stp.dhcp == true) {
    do {
    } while (Ethernet.begin(stp.mac) == 0);
    #ifdef DEBUG_PRG
      Serial.print("IP number assigned by DHCP is ");
      Serial.println(Ethernet.localIP());
    #endif
  } else Ethernet.begin(stp.mac, stp.ip, stp.dnsip, stp.gw, stp.mask);
  
  #ifdef DEBUG_PRG
    Serial.println("NTP");
  #endif
  // get NTP
  Udp.begin(localPort);
  setSyncProvider(getTimeAndDate);
  setSyncInterval(ntpSyncTime);
 
  // init webserver
  webserver.begin();
  webserver.addCommand("swt.html", &switchesCmd);
  webserver.setDefaultCommand(&switchesCmd);
  webserver.setFailureCommand(&failureCmd); 
  webserver.addCommand("stp.html", &setupCmd);
  webserver.addCommand("setuppost", &setupPost);
  
  #ifdef DEBUG_PRG
    Serial.println("433MHz");
  #endif
  // start receiver
  RemoteReceiver::init(0, REMOTE_SIGNAL_COUNT, receiveCommand); 
  
  setupSwitch = false;
}

// ----------------------------------------------------
// ----------------------- LOOP -----------------------
// ----------------------------------------------------
void loop() {
char buff[CON_BUFFER];
int len = CON_BUFFER;

  processEvents();
  webserver.processConnection(buff, &len);
}

// -------------------------------------------------------------
// ----------------------- NTP Time code -----------------------
// -------------------------------------------------------------
time_t getTimeAndDate() { 
   while (Udp.parsePacket() > 0) ;
 
   memset(packetBuffer, 0, NTP_PACKET_SIZE);
   packetBuffer[0] = 0b11100011;
   packetBuffer[1] = 0;
   packetBuffer[2] = 6;
   packetBuffer[3] = 0xEC;
   packetBuffer[12]  = 49;
   packetBuffer[13]  = 0x4E;
   packetBuffer[14]  = 49;
   packetBuffer[15]  = 52;                  
   Udp.beginPacket(timeServer, 123);
   Udp.write(packetBuffer,NTP_PACKET_SIZE);
   Udp.endPacket();
    
   uint32_t beginWait = millis();
   while (millis() - beginWait < 1500) {  
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + stp.timeZoneOffset * SECS_PER_HOUR;
    }
  }
  return 0;
}


