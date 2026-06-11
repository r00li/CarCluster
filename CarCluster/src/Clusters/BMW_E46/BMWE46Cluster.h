// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// E46 Cluster implemented by @NCPlyn
// 
// ####################################################################################################################

#ifndef BMW_E_SERIES
#define BMW_E_SERIES

#include "../../Libs/MCP_CAN/mcp_can.h" // CAN Bus Shield Compatibility Library ( https://github.com/coryjfowler/MCP_CAN_lib )
#include "../../Libs/X9C10X/X9C10X.h" // For fuel level simulation ( https://github.com/RobTillaart/X9C10X )

#include "../Cluster.h"

class BMWE46Cluster: public Cluster {
  public:
  static ClusterConfiguration clusterConfig() {
    ClusterConfiguration config;
    config.minimumCoolantTemperature = 40;
    config.maximumCoolantTemperature = 140;
    config.maximumSpeedValue = 260;
    config.maximumRPMValue = 7000;
    config.minimumFuelPotValue = 5;
    config.maximumFuelPotValue = 42; 
    config.minimumFuelPot2Value = 5;
    config.maximumFuelPot2Value = 70;
    config.isDualFuelPot = true;

    return config;
  }

  BMWE46Cluster(MCP_CAN& CAN, int fuelPotIncPin, int fuelPotDirPin, int fuelPot1CsPin, int fuelPot2CsPin, int FhandbrakePin, int FspeedPin, int FabsPin, bool fConsumption);
  void updateWithGame(GameState& game);

  private:
    MCP_CAN &CAN;
    X9C102 fuelPot = X9C102();
    X9C102 fuelPot2 = X9C102();

    int handbrakePin,speedPin,absPin;
    bool fakeConsumption;

    unsigned long last10 = 0, last20 = 0, last500 = 0; // Timer for the message loops
    unsigned long lastIndicatorL = 0,lastIndicatorR = 0; // Timer for the blinkers

    int mpgloop = 0, lastSpeed = 0, EGScounter = 0; //Looping counters and checks

    void setFuel(GameState& game);
    int mapSpeed(GameState& game);
    int mapRPM(GameState& game);
    int mapCoolantTemperature(GameState& game);
    int gearStateToDisplayCode(GearState state);

    void sendEGS1(GearState gear);
    void sendASC1(bool tractionLight);
    void sendASC3();
    void sendDME1(int rpm);
    void sendSpeed(int speed);
    void sendKBus(bool mainLights, bool highBeam, bool frontFogLight, bool rearFogLight, bool leftTurningIndicator, bool rightTurningIndicator, bool doors);
    void sendDME2(int coolantTemperature);
    void sendDME4(int rpm, bool tempWarn);
    void sendIO(bool handbrake, bool absLight);
};

#endif