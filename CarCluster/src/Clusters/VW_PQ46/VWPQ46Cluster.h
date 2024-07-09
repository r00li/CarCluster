// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef PQ46DASH
#define PQ46DASH

#include "../../Libs/MCP_CAN/mcp_can.h" // CAN Bus Shield Compatibility Library ( https://github.com/coryjfowler/MCP_CAN_lib )
#include "../../Libs/X9C10X/X9C10X.h" // For fuel level simulation ( https://github.com/RobTillaart/X9C10X )

#include "../Cluster.h"

// Known CAN IDs
#define IMMOBILIZER_ID 0x3D0 // Immobilizer
#define INDICATORS_ID 0x470 // General indicator lights
#define LIGHTS_ID 0x531 // Lights and blinkers
#define DIESEL_ENGINE_ID 0x480 // Diesel engine
#define RPM_ID 0x280 // RPM
#define SPEED_ID 0x5A0 // Speed
#define ABS_ID 0x1A0 // ABS
#define AIRBAG_ID 0x050 // Airbag
#define GEAR_ID 0x540 // Gear shifter
#define CRUISE_CONTROL_ID 0x288 // Cruise control

class VWPQ46Cluster: public Cluster {
  public:
  static ClusterConfiguration clusterConfig() {
    ClusterConfiguration config;
    config.minimumCoolantTemperature = 50;
    config.maximumCoolantTemperature = 130;
    config.maximumSpeedValue = 270;
    config.maximumRPMValue = 6000;
    config.minimumFuelPotValue = 18;
    config.maximumFuelPotValue = 83;
    config.minimumFuelPot2Value = 17;
    config.maximumFuelPot2Value = 75;
    config.isDualFuelPot = true;

    return config;
  }
  
  VWPQ46Cluster(MCP_CAN& CAN, int fuelPotIncPin, int fuelPotDirPin, int fuelPot1CsPin, int fuelPot2CsPin, int sprinklerWaterSensor, int coolantShortageSensor, int oilPressureSwitch, int handbrakeIndicator, int brakeFluidWarning);
  void updateWithGame(GameState& game);

  private:
    MCP_CAN &CAN;
    X9C102 fuelPot = X9C102();
    X9C102 fuelPot2 = X9C102();

    int sprinklerWaterSensor;
    int coolantShortageSensor;
    int oilPressureSwitch;
    int handbrakeIndicator;
    int brakeFluidWarning;

    unsigned long dashboardUpdateTime50 = 50;
    unsigned long dashboardUpdateTime500 = 500;
    unsigned long lastDashboardUpdateTime = 0; // Timer for the fast updated variables - like ABS/speed/RPM
    unsigned long lastDashboardUpdateTime500ms = 0; // Timer for slow updated variables

    int turning_lights_counter = 0;
    unsigned char seq = 0;
    int distanceCounter = 0;

    // Buffers
    unsigned char immobilizerBuffer[8] = { 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                  lightsBuffer[8] = { 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                  indicatorsBuffer[8] = { 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                  diselEngineBuffer[8] = { 0x94, 0x00, 0x4D, 0xE1, 0x18, 0x00, 0x06, 0x26 },
                  rpmBuffer[8] = { 0x49, 0x0E, 0x00, 0x00, 0x0E, 0x00, 0x1B, 0x0E },
                  speedBuffer[8] = { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAD },
                  absBuffer[8] = { 0x00, 0x40, 0x00, 0x00, 0xFE, 0xFE, 0x00, 0x00 },              
                  airbagBuffer[8] = { 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                  gearShifterBuffer[8] = { 0x90, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x02, 0x00 },
                  cruiseControlBuffer[8] = { 0x8A, 0xB4, 0x30, 0x00, 0x51, 0x46, 0x9B, 0x09 },
                  testBuff[8] = { 0x04, 0x06, 0x40, 0x00, 0xFF, 0xFE, 0x69, 0x2C };

    void sendImmobilizer();
    void sendIndicators(boolean leftBlinker, boolean rightBlinker, boolean blinkersBlinking, boolean daylightBeam, boolean highBeam, boolean frontFogLight, boolean rearFogLight, boolean batteryWarning, boolean trunkOpen, boolean doorOpen, int brightness);
    void sendDieselEngine();
    void sendRPM(int rpm);
    void sendSpeed(int speed, boolean tpmsLight, boolean espLight, boolean absLight);
    void sendABS(int speed);
    void sendGear(uint8_t gear);
    void sendAirbag(boolean seatBeltLight);
    void sendSteeringWheelControls(int button);

    void setFuel(GameState& game);
    uint8_t mapGenericGearToLocalGear(GearState inputGear);
    int mapSpeed(GameState& game);
    int mapRPM(GameState& game);
    int mapCoolantTemperature(GameState& game);
};

#endif