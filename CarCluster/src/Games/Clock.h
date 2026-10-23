// ####################################################################################################################
//
// Clock - free-running software clock for the instrument cluster (no RTC required).
// Ticks from millis(); correct it by writing the public fields (e.g. from a time set
// over serial). Values are plain (not BCD); year is 2-digit (e.g. 25 = 2025).
//
// ####################################################################################################################

#ifndef CARCLUSTER_CLOCK
#define CARCLUSTER_CLOCK

#include "Arduino.h"

class Clock {
  public:
    uint8_t hour = 12;    // 0-23
    uint8_t minute = 0;   // 0-59
    uint8_t second = 0;   // 0-59 (most clusters display only HH:MM)
    uint8_t year = 25;    // 2-digit year, e.g. 25 = 2025
    uint8_t month = 1;    // 1-12
    uint8_t day = 1;      // 1-31

    // Call every main loop iteration. Advances the time/date from elapsed millis(),
    // so the clock keeps running without an RTC. Correct it by writing the fields above.
    void tick() {
      unsigned long now = millis();
      if (lastTickMs == 0) { lastTickMs = now; return; }
      unsigned long elapsed = now - lastTickMs;          // unsigned: survives millis() wrap
      if (elapsed < 1000) return;
      unsigned long secs = elapsed / 1000;
      lastTickMs += secs * 1000;                         // keep sub-second remainder
      if (secs > 86400) secs = 86400;                    // guard against pathological catch-up
      while (secs--) { tickOneSecond(); }
    }

  private:
    unsigned long lastTickMs = 0;                        // millis() of last 1s tick (0 = not seeded)

    void tickOneSecond() {
      if (++second < 60) return;
      second = 0;
      if (++minute < 60) return;
      minute = 0;
      if (++hour < 24) return;
      hour = 0;
      if (++day <= daysInMonth(month, year)) return;
      day = 1;
      if (++month <= 12) return;
      month = 1;
      year = (uint8_t)((year + 1) % 100);
    }

    static uint8_t daysInMonth(uint8_t m, uint8_t y) {
      static const uint8_t d[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
      if (m < 1 || m > 12) return 31;
      if (m == 2 && ((2000 + y) % 4 == 0)) return 29;    // valid for years 2000-2099
      return d[m - 1];
    }
};

#endif
