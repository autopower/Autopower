# Autopower
Simple device managing power outlets - RF remote controlled. DIY device based on Arduino with Ethernet shield and 433MHz transmitter and receiver. Check out our [facebook profile](https://www.facebook.com/autopow).

## Skull & Bones
You will need these parts:
* Arduino, you can choose from Arduino with Ethernet Shield or Arduino Ethernet. If you pickup Arduino Ethernet, there is [PoE](http://en.wikipedia.org/wiki/Power_over_Ethernet) version, which allows to use Ethernet wire as a power source for Arduino, e.g. one cable to Arduino means sleek look of boxed device.
* 433MHz ASK transmitter, very cheap device. Please notice, that you need device with [ASK modulation](http://en.wikipedia.org/wiki/Amplitude-shift_keying). If you wanna use original remote controller, then you'll need 433MHz ASK receiver.
* Male to female jumper wires, if you don't wanna solder wires. Or Male to Male jumper wires and prototyping PCB board.
* Soldering iron and simple wire, if you wanna extend range of receiver. 

### Where you can get these gadgets?
* [Arduino](http://arduino.cc/en/Main/ArduinoBoardUno),
* [Ethernet shield](http://arduino.cc/en/Main/ArduinoEthernetShield),
* [Arduino Ethernet](http://arduino.cc/en/Main/ArduinoBoardEthernet),
* [433MHz receiver and transmitter](http://www.seeedstudio.com/depot/433mhz-rf-link-kit-p-127.html),
* jumper wires, check your local store with electronic gadgets or dx.com.

## Libraries you'll need to download
In any case of doubt or problem, please download libraries from [here](https://github.com/autopower/Autopower/tree/master/libraries).
* Library for [sending and receiving RF 433MHz commands](https://bitbucket.org/fuzzillogic/433mhzforarduino/wiki/Home),
* [Webduino library](https://github.com/sirleech/Webduino) for processing HTTP/HTML,
* Arduino [time library](http://www.pjrc.com/teensy/td_libs_Time.html),
* Other libraries are included in default installation of [Arduino IDE](http://arduino.cc/en/Main/Software) 1.0.5.


## How to setup hardware
Connect `DATA` pin of receiver to pin `digital 2` of Arduino (this is interrupt 0). VCC and GND connect to `5V` and `GND` on Arduino.
Transmitter `DATA` pin connect to pin `digital 7` of Arduino (if you choose other pin, please change `TRANSMITTER_PIN` in `autopower.ino`). VCC and GND connect to `Vin` and `GND` of Arduino.
Its recommended to connect 9V power adapter to enhance transmit range.

## How to setup software
To correctly setup Autpower, you must first edit, compile and upload sketch `setupap.ino` (in directory setupap/).
Please edit following variables or constants:
* if you need more that 12 characters for device name, change `#define NAMES_MAX 12`,
* if you need more or less that 16 devices, edit `#define DEV_MAX 16`,
* if you want more that 20 events, edit `#define EVENTS_MAX 20`,
* if you have more or less that 3 special (plain) RF codes edit `#define SPECIAL_MAX 3` and then edit `stp.specCode` array,
* in `setup()`, if you don't want DHCP, set `stp.dhcp` to false and edit `ip, dns, gateway` and `netmask` according tou your enviroment.
 

Please note that Arduino Uno have limited EEPROM and SRAM, so take a look at size of variables structures. Sketch `setupap.ino` writes values to EEPROM, so you MUST keep it under 1024bytes.

Then you must edit (according to changes you made above), compile and upload sketch `autopower.ino` (from autopower/ directory):
```
#define NAMES_MAX 12                    // length of device names
#define DEV_MAX 16                      // number of devices
#define EVENTS_MAX 20                   // maximum number of events
#define SPECIAL_MAX 3                   // maximum number of special codes
```

You can change more things in `autopower.ino`, e.g.:
```
#define ntpSyncTime SECS_PER_HOUR * 4   // how often sync time, in seconds
#define localPort 8888                  // local port to listen for UDP packets
#define MAX_EXTEND 3                    // how many hour you can extend switch off time, in hours
#define CON_BUFFER 48                   // size of connection buffer for webserver.processconnection
#define DIVIDER 2                       // how many switches is on one row?
#define FIRST_DOW 3                     // use this in code, 3 = Monday is day0, 4 = Sunday is day0
#define REMOTE_SIGNAL_COUNT 3           // how many times must be signal from remote controller received
#define TRANSMITTER_PIN 7               // where's the transmitter?, pin 7
```

Please DON'T change these values:
```
#define NTP_PACKET_SIZE 48              // NTP time stamp is in the first 48 bytes of the message
#define NAMELEN 15                      // maximum length of POST name
#define VALUELEN 15                     // maximum length of POST value
#define FAILED 2                        // command num of failure msg
#define UNAUTH 3                        // command num of unathorized msg
#define SWITCH_POS 0                    // where is the switching menu
#define SETUP_POS 1                     // where is the setup menu
#define NO_DEV DEV_MAX + 1              // ID for no_device
```

To select another time source, just edit this (remove comments or add your favourite time server):
```
// IPAddress timeServer(158, 195, 19, 4);  // sk.pool.ntp.org
// IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov 
const IPAddress timeServer(132, 163, 4, 102);    // time-b.timefreq.bldrdoc.gov
```

And finally admin/user name and password. Both are Base64 encoded, so feel free to edit it. Default name and password is `user:user` and `admin:admin`.
Use [this page](http://www.base64encode.org/) to obtain Base64 encoded strings.

If you have any question don't hesitate and use out Facebook profile to ask the questions.

##Supported RC power outlets
Supported remote controlled power outlets uses tribits switching, e.g. http://www.tipa.eu/en/radio-control-switch-kit-ta35/d-86641/
You can setup these outlets and remote controller by 10 DIP switches, first 5 DIPs for channel and rest of them, for address – A to E.
Virtually you can control 32*5 = 160 power oulets.

## Addressing
This is simple, setup your outlets and read out channel and address. Channel is first 5 DIP switches. If everything is off then channel is 0. Everything on, means you selected channel 31. Channel number is calculated as bits starting from least significant (16 + 8 + 4 + 2 + 1 = 31).
Selecting address is even simplier. Just turn on desired DIP switch. First one is for A, last one for E.

## Operating modes
You can operate outlet in one of these three modes:
* Normal [Nm], operated like classic RC outlet, can be switched on and off via button on webpage. Outlet can be cycled via simple POST request.
* Time event driven [Tm], outlet can be switched on and off, in defined time and day.
* Counted [Cn], outlet is switched on every X minutes for defined moment of Y minutes.


If you need extend funcionality of outlet, you can do it by setting flag:
* Normal [Nm], no extension to functionality,
* Vacation free [Vc], outlet will be switched (if Time event driven or Counted) even if Autopower is in vacation mode, indicated by # character.,
* Auto Off [Ao], outlet will be turned off after time period (edit this in setup), indicated by ! character,
* Special [Sp], outlet will be turned on or off viac `specCode` array. Position of signal in `specCode` array is done by choosing channel (e.g. for position 0 in `specCode` choose channel 0) in setup dialog. Indicated by S character.

 
If outlet is in Time event driven mode, it's on time can be extended byt plus sign displayed next to the outlet name.

## Other features
If you click on the clock, time will be renewed by time server.
If you click on word `normal`, Autopower will be switched to vacation mode for 2 days (weekend) and this will be indicated by word `vacation`. If you again click on word vacation, vacation period will be extended by two days. if you need turn off vacation mode, please go to the setup and enter `0` (zero) into the vacation textbox and click save.

## Commands
To turn on or off device 10 just visit this webpage `http://ip_of_your_autopower/swt.html?cmd=9` (devices are numbered from 0, thus 9 is the 10th device).
To turn on vacation mode for 10 days, just type in your browser `http://ip_of_your_autopower/swt.html?vac=10`. If you wanna turn off vacation mode use this: `http://ip_of_your_autopower/swt.html?vac=0`.

## Security
If you asking why we using user/admin name and password an no SSL, answer is simple. Arduino Uno/Ethernet is not capable of SSL. So why user names and password? Because many family members can switch outlets, but few of them are able to setup these (you know what we mean :)). Yes, yes we know about sniffing and all that hacking skills. But remember, RF signals @433MHz are easy "hackable" via standalone simple remote controller. 

## Examples
* Setup outlet 1, channel 31, address A, time driven, no flag.
* Setup outlet 2, channel 31, address B, normal, auto off.
* Setup outlet 3, channel 0, address A, time drive, special.
* Setup event 1, with outlet 1, turn on everyday at 8:00, turn off everyday at 20:00.
* Setup event 2, with outlet 2, turn on whatever, turn off 01:20, no day selected. 
* Setup event 3, with outlet 3, turn on Sat, Sun, 20:30, turn off Sat, Sun, 22:30.


If you manually turn on outlet 2, it'll be turned off after 1 hour and 20 minutes. Use it for example cell phone charger.
At Sat and Sun, 20:30 Autopower will send plain signal from `stp.specCode` position 0. And the same signal will be send at 22:30. Ideal for LED dimmers etc.
Everyday at 8:00 will Autopower turn on outlet 1, which will be turned off at 20:00.
