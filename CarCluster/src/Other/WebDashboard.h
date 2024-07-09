// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef WEB_DASHBOARD
#define WEB_DASHBOARD

#include "Arduino.h"
#include "WiFi.h" // Arduino system library (part of ESP core)

#include "../Libs/AsyncTCP/AsyncTCP.h" // Requirement for ESP-DASH ( https://github.com/mathieucarbou/AsyncTCP )
#include "../Libs/ESPAsyncWebServer/ESPAsyncWebServer.h"  // Requirement for ESP-DASH ( https://github.com/mathieucarbou/ESPAsyncWebServer )
#include "../Libs/ESP-DASH/ESPDash.h" // Web dashboard ( https://github.com/ayushsharma82/ESP-DASH )

#include "../Games/GameSimulation.h"


class WebDashboard {
  WebDashboard(const WebDashboard &other) = delete;
  WebDashboard(WebDashboard &&other) = delete;
  WebDashboard &operator=(const WebDashboard &other) = delete;
  WebDashboard &operator=(WebDashboard &&other) = delete;

  public:
    WebDashboard(GameState& game, int serverPort, unsigned long webDashboardUpdateInterval);
    void begin();
    void update();

  private:
    GameState &gameState;
    AsyncWebServer server;
    ESPDash dashboard;
    unsigned long webDashboardUpdateInterval;

    unsigned long lastWebDashboardUpdateTime = 0;

    // Cards
    Card introCard;
    Card speedCard;
    Card rpmCard;
    Card fuelCard;
    Card highBeamCard;
    Card fogLampCard;
    Card frontFogLampCard;
    Card leftTurningIndicatorCard;
    Card rightTurningIndicatorCard;
    Card mainLightsCard;
    Card doorOpenCard;
    Card dscActiveCard;
    Card absLightCard;
    Card gearCard;
    Card backlightCard;
    Card coolantTemperatureCard;
    Card handbrakeCard;
    Card button1Card;
    Card button2Card;
    Card button3Card;
    Card ignitionCard;
    Card driveModeCard;
    Card outdoorTemperatureCard;
    Card indicatorsBlinkCard;

    // Card val0Card(&dashboard, SLIDER_CARD, "VAL0", "", 0, 255);
    // Card val1Card(&dashboard, SLIDER_CARD, "VAL1", "", 0, 255);
    // Card val2Card(&dashboard, SLIDER_CARD, "VAL2", "", 0, 255);
    // Card val3Card(&dashboard, SLIDER_CARD, "VAL3", "", 0, 255);
    // Card val4Card(&dashboard, SLIDER_CARD, "VAL4", "", 0, 255);
    // Card val5Card(&dashboard, SLIDER_CARD, "VAL5", "", 0, 255);
    // Card val6Card(&dashboard, SLIDER_CARD, "VAL6", "", 0, 255);
    // Card val7Card(&dashboard, SLIDER_CARD, "VAL7", "", 0, 255);
    // uint8_t val0 = 0, val1 = 0, val2 = 0, val3 = 0, val4 = 0, val5 = 0, val6 = 0, val7 = 0;
};

#endif