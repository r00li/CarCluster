// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef F_SERIES_DASH
#define F_SERIES_DASH

#include <mcp_can.h>
#include "CRC8.h"
#include "../Cluster.h"

class BMWFSeriesCluster: public Cluster {

  // Do not ever send 0x380 ID - that is VIN number

  public:
  static ClusterConfiguration clusterConfig() {
    ClusterConfiguration config;
    return config;
  }

  BMWFSeriesCluster(MCP_CAN& CAN, bool isCarMini);
  void updateWithGame(GameState& game);
  void updateLanguageAndUnits();

  uint8_t inFuelRange[3] = {};
  uint8_t outFuelRange[3] = {};

  private:
  MCP_CAN &CAN;
  CRC8 crc8Calculator;

  unsigned long dashboardUpdateTime100 = 100;
  unsigned long dashboardUpdateTime1000 = 500;
  unsigned long lastDashboardUpdateTime = 0; // Timer for the fast updated variables
  unsigned long lastDashboardUpdateTime1000ms = 0; // Timer for slow updated variables

  uint8_t counter4Bit = 0;
  uint8_t count = 0;
  uint16_t distanceTravelledCounter = 0;
  bool isCarMini = false;

  void sendIgnitionStatus(boolean ignition);
  void sendSpeed(int speed);
  void sendRPM(int rpm, int manualGear);
  void sendAutomaticTransmission(int gear);
  void sendBasicDriveInfo(int engineTemperature);
  void sendParkBrake(boolean handbrakeActive);
  void sendFuel(int fuelQuantity, uint8_t inFuelRange[], uint8_t outFuelRange[], boolean isCarMini);
  void sendDistanceTravelled(int speed);
  void sendBlinkers(boolean leftTurningIndicator, boolean rightTurningIndicator);
  void sendLights(boolean mainLights, boolean highBeam, boolean rearFogLight, boolean frontFogLight);
  void sendBacklightBrightness(uint8_t brightness);
  void sendAlerts(boolean offroad, boolean doorOpen, boolean handbrake, boolean isCarMini);
  void sendSteeringWheelButton(int buttonEvent);
  void sendDriveMode(uint8_t driveMode);
};

#endif