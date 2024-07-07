// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef WIFI_FUNCTIONS
#define WIFI_FUNCTIONS

#include "Arduino.h"
#include "WiFi.h" // Arduino system library (part of ESP core)
#include "../Libs/WiFiManager/WiFiManager.h" // For easier wifi management ( https://github.com/tzapu/WiFiManager )

class WifiFunctions {
  public:
    void begin(char const *apName, char const *apPassword, int apTimeout);
};

#endif