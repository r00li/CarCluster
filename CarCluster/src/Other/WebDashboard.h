// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef WEB_DASHBOARD
#define WEB_DASHBOARD

#include "Arduino.h"

#include "mongoose/mongoose.h"
#include "mongoose/mongoose_glue.h"
#include "../Games/GameSimulation.h"


class WebDashboard {
  WebDashboard(const WebDashboard &other) = delete;
  WebDashboard(WebDashboard &&other) = delete;
  WebDashboard &operator=(const WebDashboard &other) = delete;
  WebDashboard &operator=(WebDashboard &&other) = delete;

  public:
    WebDashboard(GameState& game, unsigned long webDashboardUpdateInterval);
    void update();
    void getState(struct state *data);
    void setState(struct state *data);
    void steeringWheelAction(struct mg_str params);

  private:
    GameState &gameState;
    unsigned long webDashboardUpdateInterval;
    unsigned long lastWebDashboardUpdateTime = 0;

    const char* mapGenericGearToLocalGear(GearState inputGear);
    GearState mapLocalGearToGenericGear(const char *gear);
    const char* mapGenericDriveModeToLocalDriveMode(uint8_t driveMode);
    uint8_t mapLocalDriveModeToGenericDriveMode(const char *driveMode);
};

#endif