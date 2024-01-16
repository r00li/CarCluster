// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef F_SERIES_DASH
#define F_SERIES_DASH

#include <mcp_can.h>
#include "CRC8.h"

class FSeriesDash {

  // Do not ever send 0x380 ID - that is VIN number

  public:
    FSeriesDash(MCP_CAN& CAN);
    void updateWithState(int speed,
                         int rpm,
                         int fuelPercentage,
                         uint8_t inFuelRange[],
                         uint8_t outFuelRange[],
                         boolean isCarMini,
                         uint8_t backlightBrightness,
                         boolean leftTurningIndicator,
                         boolean rightTurningIndicator,
                         uint8_t gear,
                         int engineTemperature,
                         boolean handbrake,
                         boolean highBeam,
                         boolean mainBeam,
                         boolean rearFogLight,
                         boolean frontFogLight,
                         boolean doorOpen,
                         boolean espLight,
                         boolean ignition,
                         uint8_t driveMode);
    void sendSteeringWheelButton();
    void updateLanguageAndUnits();

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

    uint8_t buttonEventToProcess = 0;

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