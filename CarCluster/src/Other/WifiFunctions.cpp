// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "WifiFunctions.h"

void WifiFunctions::begin(char const *apName, char const *apPassword, int apTimeout) {
  // Connect to wifi
  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConfigPortalTimeout(apTimeout);
  bool res = wm.autoConnect(apName, apPassword);
  if(!res) {
      Serial.println("Wifi Failed to connect");
  } else {
      Serial.println("Wifi connected...yeey :)");
  }

  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}