// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef BMW_E_SERIES
#define BMW_E_SERIES

#include "../../Libs/MCP_CAN/mcp_can.h" // CAN Bus Shield Compatibility Library ( https://github.com/coryjfowler/MCP_CAN_lib )
#include "../../Libs/X9C10X/X9C10X.h" // For fuel level simulation ( https://github.com/RobTillaart/X9C10X )

#include "../Cluster.h"

class BMWESeriesCluster: public Cluster {
  public:
  static ClusterConfiguration clusterConfig() {
    ClusterConfiguration config;
    config.minimumCoolantTemperature = 50;
    config.maximumCoolantTemperature = 130;
    config.maximumSpeedValue = 240;
    config.maximumRPMValue = 6000;
    config.minimumFuelPotValue = 18;
    config.maximumFuelPotValue = 83;
    config.minimumFuelPot2Value = 17;
    config.maximumFuelPot2Value = 75;
    config.isDualFuelPot = true;

    return config;
  }

  BMWESeriesCluster(MCP_CAN& CAN, int fuelPotIncPin, int fuelPotDirPin, int fuelPot1CsPin, int fuelPot2CsPin, int handbrakeIndicatorPin);
  void updateWithGame(GameState& game);

  private:
    MCP_CAN &CAN;
    X9C102 fuelPot = X9C102();
    X9C102 fuelPot2 = X9C102();

    int handbrakeIndicator;

    unsigned long dashboardUpdateTimeShort = 100;
    unsigned long dashboardUpdateTimeLong = 500;
    unsigned long lastDashboardUpdateTime = 0; // Timer for the fast updated variables
    unsigned long lastDashboardUpdateTimeLong = 0; // Timer for slow updated variables

    uint8_t speedFrame[8] = {0x13, 0x4D, 0x46, 0x4D, 0x33, 0x4D, 0xD0, 0xFF};
    uint16_t previousSpeedValue = 0;
    uint8_t absFrame[8] = {0x00, 0xE0, 0xB3, 0xFC, 0xF0, 0x43, 0x00, 0x65};
    uint8_t counter = 0;
    uint8_t counterGear = 0x0;

    void setFuel(GameState& game);
    uint8_t mapGenericGearToLocalGear(GearState inputGear);
    int mapSpeed(GameState& game);
    int mapRPM(GameState& game);
    int mapCoolantTemperature(GameState& game);

    void sendIgnition(bool ignition);
    void sendRPM(int rpm);
    void sendSpeed(int speed, int deltaTime);
    void sendSteeringWheel();
    void sendAirbagCounter();
    void sendABSCounter();
    void sendABS(uint8_t a1, uint8_t a2);
    void sendLights(bool mainLights, bool highBeam, bool frontFogLight, bool rearFogLight);
    void sendEngineTemperature(int coolantTemperature);
    void sendHanbrake(bool handbrake);
    void sendSteeringWheelControls(int button);
    void sendGearStatus(int gear);
    void sendDoorStatus(bool doorOpen);
    void sendBacklightBrightness(int brightness);
    void sendBlinkers(bool leftBlinker, bool rightBlinker);
    void sendTime();
};

#endif