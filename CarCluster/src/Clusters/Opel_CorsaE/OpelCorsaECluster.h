// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

// load of barnacles (https://github.com/SkodaSuperbB6), 20.09.2026

#ifndef CORSADASH
#define CORSADASH

#include "../../Libs/MCP_CAN/mcp_can.h" // CAN Bus Shield Compatibility Library ( https://github.com/coryjfowler/MCP_CAN_lib )

#include "../Cluster.h"

#include <Preferences.h> // Preferences.h library for saving the odometer value

class OpelCorsaECluster: public Cluster {
  public:
  static ClusterConfiguration clusterConfig() {
    ClusterConfiguration config;
    config.minimumCoolantTemperature = 50;
    config.maximumCoolantTemperature = 130;
    config.maximumSpeedValue = 220;
    config.maximumRPMValue = 6000;

    return config;
  }
  
  OpelCorsaECluster(MCP_CAN& CAN, uint32_t configuredMileage, bool odometerEnabled, int diagTestMode, int languageSet);
  void updateWithGame(GameState& game);

  private:
    MCP_CAN &CAN;

    unsigned long dashboardUpdateTime50 = 50;
    unsigned long dashboardUpdateTime250 = 250;
    unsigned long dashboardUpdateTime1000 = 1000;
    unsigned long dashboardUpdateTime3000 = 3000;

    unsigned long lastDashboardUpdateTime = 0; 
    unsigned long lastDashboardUpdateTime250ms = 0;
    unsigned long lastDashboardUpdateTime1000ms = 0;
    unsigned long lastDashboardUpdateTime3000ms = 0;

    int turning_lights_counter = 0;
    unsigned char seq = 0;

    Preferences odometerPrefs;

    uint32_t savedMileage = 0;
    uint32_t configuredMileage = 0;
    uint32_t odometerRaw = 0;
    uint32_t distanceMeters = 0;
    uint32_t lastMillis = 0;
    uint32_t lastSavedMileage = 0;
    uint32_t lastSentMileage = 0;

    bool odometerInitialized = false;
    bool odometerEnabled = false;

    int diagTestMode = 0;
    int languageSet = 0;

    void sendSpeed(float speed);
    void sendRPM(int rpm);
    void sendABS();
    void sendTCS();

    void sendDoorStatus(bool doorOpen, bool doorOpenFr, bool doorOpenRl, bool doorOpenRr, bool trunkOpen, bool hoodOpen);
    void sendLights(bool leftBlinker, bool rightBlinker, bool blinkersBlinking, bool lowBeam, bool highBeam, bool rearFog);
    void sendBacklight(int brightness);
    void sendFuel(int fuelAmount, int rpm, int configMaxRpm);
    void sendCoolant(int coolantTemperature);
    void sendEngine(bool checkEngine);
    void sendHandbrake(bool handbrake);
    void sendAirbag(bool seatbelt);
    void sendTPMS();
    void sendOdometer(float speed);
    void initializeOdometer();

    void sendDiagTest();
    void sendLanguage();
    void sendGmlan();
    void sendTest();

    float mapSpeed(GameState& game);
    int mapRPM(GameState& game);
    int mapCoolantTemperature(GameState& game);
};

#endif