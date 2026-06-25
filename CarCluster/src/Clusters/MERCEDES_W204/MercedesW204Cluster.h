// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef W204_DASH
#define W204_DASH

#include "../../Libs/MultiMap/MultiMap.h" // For fuel level calculation - supports non linear mapping found on BMW clusters ( https://github.com/RobTillaart/MultiMap )
#include "../../Libs/MCP_CAN/mcp_can.h" // CAN Bus Shield Compatibility Library ( https://github.com/coryjfowler/MCP_CAN_lib )

#include "../Cluster.h"

#define lo8(x) (uint8_t)((x) & 0xFF)
#define hi8(x) (uint8_t)(((x)>>8) & 0xFF)

class MercedesW204Cluster: public Cluster {
  public:
  static ClusterConfiguration clusterConfig() {
    ClusterConfiguration config;
    config.minimumCoolantTemperature = 40;
    config.maximumCoolantTemperature = 120;
    config.maximumSpeedValue = 260;
    config.maximumRPMValue = 6000;
    return config;
  }

  MercedesW204Cluster(MCP_CAN& CAN, MCP_CAN& CAN2);
  void updateWithGame(GameState& game);
  void sendSteeringWheelControls(int button);

  private:
  MCP_CAN &CAN;
  MCP_CAN& CAN2;

  unsigned long dashboardUpdateTime100 = 100;
  unsigned long lastDashboardUpdateTime = 0; // Timer for the fast updated variables

  uint8_t count = 0;

  uint8_t mapGenericGearToLocalGear(GearState inputGear);
  int mapSpeed(GameState& game);
  int mapRPM(GameState& game);
  int mapCoolantTemperature(GameState& game);

  void sendIgnition(bool ignition);
  void sendBasicDriveParameters(int speed, int rpm);
  void sendGear(int gear);
  void sendCoolantTemperature(int temperature);
  void sendLights(bool mainLights, bool highBeam, bool frontFogLight, bool rearFogLight, bool backlightOn, bool parkingBrake, int outdoorTemp);
  void sendBlinkers(bool leftBlinker, bool rightBlinker);
  void sendFuel(int fuelPercentage);
  void sendAbs(bool absOn, bool tractionOn);
  void sendDoorStatus(bool open);
  void sendOthers();
};

#endif