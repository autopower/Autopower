#if defined(ARDUINO) && ARDUINO > 18
  #include <SPI.h>
#endif
#include "simpleTimer.h"

simpleTimer::simpleTimer()
{
}

byte simpleTimer::getType(byte idx) {
  return (dev.type[idx] & TYPE_MASK);
}

byte simpleTimer::getMode(byte idx) {
  return (dev.type[idx] >> MODE_SHIFT);
}
