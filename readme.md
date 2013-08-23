Autopower

=========


Simple device managing power outlets – remote controlled. DIY device based on Arduino with Ethernet shield and 433MHz transmitter and receiver.

Check (and Like! :)) our facebook profile: https://www.facebook.com/autopow


Skull & Bones
-------------
You will need these parts:
- Arduino, you can choose from Arduino with Ethernet Shield or Arduino Ethernet. If you pickup Arduino Ethernet, there is PoE
  (Power over Ethernet, http://en.wikipedia.org/wiki/Power_over_Ethernet) version, which allows to use Ethernet wire as a power
  source for Arduino, e.g. one cable to Arduino means sleek look of boxed device.
- 433MHz ASK transmitter, very cheap device. Please notice, that you need device with ASK modulation (http://en.wikipedia.org/wiki/Amplitude-shift_keying).
  If you wanna use original remote controller, then you'll need 433MHz ASK receiver.
- Male to female jumper wires, if you don't wanna solder wires. Or Male to Male jumper wires and prototyping PCB board.
- Soldering iron and simple wire, if you wanna extend range of receiver. 

Where you can get these gadgets?
Arduino: http://arduino.cc/en/Main/ArduinoBoardUno
Ethernet shield: http://arduino.cc/en/Main/ArduinoEthernetShield
Arduino Ethernet: http://arduino.cc/en/Main/ArduinoBoardEthernet
433MHz receiver and transmitter: http://www.seeedstudio.com/depot/433mhz-rf-link-kit-p-127.html
jumper wires, check your local store with electronic gadgets or dx.com.


Libraries you'll need to download
---------------------------------
Library for sending and receiving RF 433MHz commands: https://bitbucket.org/fuzzillogic/433mhzforarduino/wiki/Home 
Webduino library for processing HTTP/HTML: https://github.com/sirleech/Webduino
Arduino time library: http://playground.arduino.cc/Code/time
Other libraries are included in default installation of Arduino IDE 1.0.5 (http://arduino.cc/en/Main/Software).


How to
------
To correctly setup Autpower, you must first edit, compile and upload sketch setupap.ino (in directory setupap/).
Please edit following variables or constants:
- if you need more that 12 characters for device name, change #define NAMES_MAX 12,
- if you need more or less that 16 devices, edit #define DEV_MAX 16,
- if you want more that 20 events, edit #define EVENTS_MAX 20,
- if you have more or less that 3 special (plain) RF codes edit #define SPECIAL_MAX 3 and then edit stp.specCode array,
- in setup(), if you don't want DHCP, set stp.dhcp to false and edit ip, dns, gateway and netmask according tou your enviroment.
Please note that Arduino Uno have limited SRAM, so take a look at size of variables structures.

Then you must edit (according to changes you made above), compile and upload sketch autopower.ino (from autopower/ directory):
#define NAMES_MAX 12                    // length of device names

#define DEV_MAX 16                      // number of devices

#define EVENTS_MAX 20                   // maximum number of events

#define SPECIAL_MAX 3                   // maximum number of special codes


You can change more things in autopower.ino, e.g.:
#define ntpSyncTime SECS_PER_HOUR * 4   // how often sync time, in seconds

#define localPort 8888                  // local port to listen for UDP packets

#define MAX_EXTEND 3                    // how many hour you can extend switch off time, in hours

#define CON_BUFFER 48                   // size of connection buffer for webserver.processconnection

#define DIVIDER 2                       // how many switches is on one row?

#define FIRST_DOW 3                     // use this in code, 3 = Monday is day0, 4 = Sunday is day0

#define REMOTE_SIGNAL_COUNT 3           // how many times must be signal from remote controller received

#define TRANSMITTER_PIN 7               // where's the transmitter?, pin 7

Please DON'T change these values:
#define NTP_PACKET_SIZE 48              // NTP time stamp is in the first 48 bytes of the message

#define NAMELEN 15                      // maximum length of POST name

#define VALUELEN 15                     // maximum length of POST value

#define FAILED 2                        // command num of failure msg

#define UNAUTH 3                        // command num of unathorized msg

#define SWITCH_POS 0                    // where is the switching menu

#define SETUP_POS 1                     // where is the setup menu

#define NO_DEV DEV_MAX + 1              // ID for no_device


To select another time source, just edit this (remove comments or add your favourite time server):
// IPAddress timeServer(158, 195, 19, 4);  // sk.pool.ntp.org

// IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov 
const 
IPAddress timeServer(132, 163, 4, 102);    // time-b.timefreq.bldrdoc.gov

And finally admin/user name and password. Both are Base64 encoded, so feel free to edit it. Default name and password is "user:user" and "admin:admin".
Use this page: http://www.base64encode.org/ to obtain Base64 encoded strings.

If you have any question don't hesitate and use out Facebook profile to ask the questions.

