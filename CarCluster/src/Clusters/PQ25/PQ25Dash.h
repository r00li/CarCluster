// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef PQ25DASH
#define PQ25DASH

#include <mcp_can.h>

// Known CAN IDs
#define IMMOBILIZER_ID 0x3D0 // Immobilizer
#define INDICATORS_ID 0x470 // General indicator lights
#define DIESEL_ENGINE_ID 0x480 // Diesel engine
#define RPM_ID 0x280 // RPM
#define SPEED_ID 0x5A0 // Speed
#define ABS_ID 0x1A0 // ABS
#define AIRBAG_ID 0x050 // Airbag
#define GEAR_ID 0x540 // Gear shifter
#define CRUISE_CONTROL_ID 0x288 // Cruise control

class PQ25Dash {
  public:
    PQ25Dash(MCP_CAN& CAN);
    void updateWithState(int speed,
                         int rpm,
                         uint8_t backlightBrightness,
                         boolean leftBlinker,
                         boolean rightBlinker,
                         boolean blinkersBlinking,
                         boolean highBeam,
                         boolean frontFogLight,
                         boolean batteryWarning,
                         boolean trunkOpen,
                         boolean doorOpen,
                         boolean tpmsLight,
                         boolean espLight,
                         boolean absLight,
                         uint8_t gear);
    //void updateTestBuffer(uint8_t val0, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5, uint8_t val6, uint8_t val7);
    //void sendSteeringWheelControls(int button);

  private:
    MCP_CAN &CAN;

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
    void sendIndicators(boolean leftBlinker, boolean rightBlinker, boolean blinkersBlinking, boolean highBeam, boolean frontFogLight, boolean batteryWarning, boolean trunkOpen, boolean doorOpen, int brightness);
    void sendDieselEngine();
    void sendRPM(int rpm);
    void sendSpeed(int speed, boolean tpmsLight, boolean espLight, boolean absLight);
    void sendABS(int speed);
    void sendGear(uint8_t gear);
    void sendAirbag(boolean seatBeltLight);
};

#endif