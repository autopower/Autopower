#if defined(ARDUINO) && ARDUINO > 18
  #include <SPI.h>
#endif
#include "simpleTimer.h"

simpleTimer::simpleTimer() {
  vacEnd = 0;
}


byte simpleTimer::getType(byte idx) {
  return (dev.type[idx] & TYPE_MASK);
}


byte simpleTimer::getMode(byte idx) {
  return (dev.type[idx] >> MODE_SHIFT);
}

byte simpleTimer::getChannel(byte idx) {
  return (dev.adr[idx] & CHANNEL_MASK);
}

byte simpleTimer::getAddress(byte idx) {
  return (dev.adr[idx] >> ADDRESS_SHIFT); 
}

void simpleTimer::setVacationMode(int days) {
  if (days > 0) vacEnd = now() + (days * SECS_PER_DAY);
}


// check for automatically switched off outlet, if so set variable. Outlet cannot be time driven or counted
void simpleTimer::setAutoOff(byte idx) {
  // its simple, must be type 0 and mode autooff (2), so 2bits=00, 2bits=10
  for (byte i = 0; i  < EVENTS_MAX; i++) {
    if (events.device[i] == idx && getMode(idx) == MODE_AUTOOFF) {              // if you find first events with this outlet and this outlet is AutoOff setup values and exit loop
      dev.count[idx] = now() + (hour(events.t_off[i]) * SECS_PER_HOUR) + (minute(events.t_off[i]) * SECS_PER_MIN);
      break;
    }
  }
} 


// switch device on or off
void simpleTimer::switchOnOff(byte idx, boolean onoff) {
  dev.status[idx] = onoff;
  // disable receiving, not to interfere with actual command
  // switch TBI
  // enable receiving
}


#ifdef SWITCH_ALL
void simpleTimer::switchAll(boolean onoff) {
  for (byte i = 0; i < DEV_MAX; i++) switchOnOff(i, onoff);
}
#endif


// is this that time we can switch on or off?
boolean simpleTimer::isTime(byte eidx, byte didx, boolean onoroff) {
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

void simpleTimer::processEvents() {
int cM, nM, oM;
byte mode, dIdx;

  nw = now();
  // return DOW in order, accoring to FIRST_DOW (Monday=0..Sunday=6)
  nowD = ((nw / 86400L) + FIRST_DOW) % 7;
  nowH = hour(nw);
  nowM = minute(nw);
  
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
