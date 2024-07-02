// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef WIFI_FUNCTIONS
#define WIFI_FUNCTIONS

#include <Arduino.h>
#include "WiFiManager.h"        // For easier wifi management (install through library manager: WiFiManager by tzapu - tested using 2.0.16-rc.2)
#include <WiFi.h>               // Arduino system library (part of ESP core)

class WifiFunctions {
  public:
    void begin(char const *apName, char const *apPassword, int apTimeout);
};

#endif