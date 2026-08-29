// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

// load of barnacles (https://github.com/SkodaSuperbB6), 06.08.2026

#ifndef CORSADASH
#define CORSADASH

#include "../../Libs/MCP_CAN/mcp_can.h" // CAN Bus Shield Compatibility Library ( https://github.com/coryjfowler/MCP_CAN_lib )

#include "../Cluster.h"

#include <Preferences.h> // Preferences.h library for saving the odometer value

// Known 11-bit GMLAN IDs
#define GMLAN_ID 0x100 // Keep alive
#define DIAG_ID 0x24C // Diagnostics test 

// Known 29-bit GMLAN IDs
#define DOOR_ID 0x10630000 // Front left door status
#define DOOR_FR_ID 0x0C2F6040 
#define DOOR_RL_ID 0x0C2F8040 
#define DOOR_RR_ID 0x0C2FA040 
#define TRUNK_ID 0x106AA040 
#define HOOD_ID 0x10728040 

#define SPEED_ID 0x10210040
#define WHEEL_ID 0x106B8040 // Rotation speed for each wheel of the car
#define RPM_ID 0x102CA040
#define TRANS_ID 0x102CC040 // Transmission? Powertrain?
#define ABS_ID 0x1022A040
#define TCS_ID 0x10240040

#define LIGHTS_ID 0x1020C040 // Blinker & headlights
#define BACKLIGHT_ID 0x10644040 // Cluster brightness
#define FUEL_ID 0x10800040 
#define FUEL_CONSUMPTION_ID 0x10264040
#define FUEL_RANGE_ID 0x102D0040
#define FUEL_MESSAGE_ID 0x10806040 // Messages relevant to fuel
#define COOLANT_ID 0x102E0040
#define ENGINE_ID 0x10780040 // Check engine light & other messages
#define HANDBRAKE_ID 0x103B4040
#define AIRBAG_ID 0x10330058
#define TIRE_LEARN_ID 0x103DC040 // Tire calibration message
#define TIRE_PRESSURE_ID 0x103D4040

#define ODOMETER_ID 0x106C0040
// #define IGNITION_ID 0x10242040 // Seemingly useless, ignition pin from cluster does the job
// #define STARTER_ID 0x1045C060
// #define VIN_ID 0x10EC8040 // VIN? DO NOT TOUCH!! Odometer shows dashes after sending this
// #define CHIME_ID 0x10400060 // Useless, it is played through the car's stereo, cluster doesn't have speaker

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
  
  OpelCorsaECluster(MCP_CAN& CAN, uint32_t configuredMileage, bool odometerEnabled, int diagTestMode);
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

    // Buffers
    unsigned char doorBuffer[1] = { 0x00 },
                  doorFrBuffer[1] = { 0x00 },
                  doorRlBuffer[1] = { 0x00 },
                  doorRrBuffer[1] = { 0x00 },
                  trunkBuffer[1] = { 0x00 },
                  hoodBuffer[1] = { 0x00 },

                  speedBuffer[8] = { 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00 },
                  wheelBuffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                  rpmBuffer[8] = { 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 },
                  transBuffer[8] = { 0x00, 0x00, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x00 },
                  absBuffer[7] = { 0x00, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00 },
                  tcsBuffer[8] = { 0x83, 0xFA, 0x88, 0x01, 0xFF, 0x31, 0x00, 0x00 },
                  lightsBuffer[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 },
                  backlightBuffer[3] = { 0x02, 0x00, 0x00 },

                  fuelBuffer[6] = { 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00 },
                  fuelRangeBuffer[8] = { 0x00, 0x00, 0x01, 0xA0, 0xFF, 0x00, 0x00, 0xFF },
                  fuelConsumptionBuffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                  fuelMessageBuffer[5] = { 0x00, 0x00, 0x00, 0x00, 0x14 },

                  coolantBuffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                  engineBuffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00 },
                  handbrakeBuffer[1] = { 0x04 },
                  airbagBuffer[6] = { 0x73, 0x73, 0x73, 0x00, 0x00, 0x00 },
                  tireLearnBuffer[2] = { 0x05, 0x85 },
                  tirePressureBuffer[6] = { 0x24, 0x24, 0x38, 0x38, 0x38, 0x38 },
                  odometerBuffer[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 },

                  diagNeedleBuffer[8] = { 0x03, 0xAE, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 },
                  diagChristmasBuffer[8] = { 0x03, 0xAE, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00 },
                  diagDisplayBuffer[8] = { 0x03, 0xAE, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00 },
                  diagExitBuffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

                  gmlanEmptyBuffer[1] = { 0 };

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
    void sendGmlan();
    void sendTest();

    float mapSpeed(GameState& game);
    int mapRPM(GameState& game);
    int mapCoolantTemperature(GameState& game);
};

#endif