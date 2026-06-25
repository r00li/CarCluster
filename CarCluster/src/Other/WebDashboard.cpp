// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "WebDashboard.h"

WebDashboard::WebDashboard(GameState &game, unsigned long webDashboardUpdateInterval): gameState(game) {
  this->webDashboardUpdateInterval = webDashboardUpdateInterval;
}

void WebDashboard::getState(struct state *data) {
  data->speed = gameState.speed;
  data->maximumSpeed = gameState.configuration.maximumSpeedValue;
  data->rpm = gameState.rpm;
  data->maximumRPM = gameState.configuration.maximumRPMValue;
  data->fuel = gameState.fuelQuantity;
  data->high_beam = gameState.highBeam;
  data->fog_rear = gameState.rearFogLight;
  data->fog_front = gameState.frontFogLight;
  data->left_indicator = gameState.leftTurningIndicator;
  data->right_indicator = gameState.rightTurningIndicator;
  data->main_lights = gameState.mainLights;
  data->door_open = gameState.doorOpen;
  data->dsc = gameState.offroadLight;
  data->abs = gameState.absLight;
  strcpy(data->gear, mapGenericGearToLocalGear(gameState.gear));
  data->backlight = gameState.backlightBrightness;
  data->coolant_temp = gameState.coolantTemperature;
  data->minimumCoolantTemp = gameState.configuration.minimumCoolantTemperature;
  data->maximumCoolantTemp = gameState.configuration.maximumCoolantTemperature;
  data->handbrake = gameState.handbrake;
  data->ignition = gameState.ignition;
  strcpy(data->drive_mode, mapGenericDriveModeToLocalDriveMode(gameState.driveMode));
  data->outdoor_temp = gameState.outdoorTemperature;
  data->indicators_blink = gameState.turningIndicatorsBlinking;
}

void WebDashboard::setState(struct state *data) {
  gameState.speed = data->speed;
  gameState.rpm = data->rpm;
  gameState.fuelQuantity = data->fuel;
  gameState.highBeam = data->high_beam;
  gameState.rearFogLight = data->fog_rear;
  gameState.frontFogLight = data->fog_front;
  gameState.leftTurningIndicator = data->left_indicator;
  gameState.rightTurningIndicator = data->right_indicator;
  gameState.mainLights = data->main_lights;
  gameState.doorOpen = data->door_open;
  gameState.offroadLight = data->dsc;
  gameState.absLight = data->abs;
  gameState.gear = mapLocalGearToGenericGear(data->gear);
  gameState.backlightBrightness = data->backlight;
  gameState.coolantTemperature = data->coolant_temp;
  gameState.handbrake = data->handbrake;
  gameState.ignition = data->ignition;
  gameState.driveMode = mapLocalDriveModeToGenericDriveMode(data->drive_mode);
  gameState.outdoorTemperature = data->outdoor_temp;
  gameState.turningIndicatorsBlinking = data->indicators_blink;
}

void WebDashboard::steeringWheelAction(struct mg_str params) {
  if (params.len >= 1) {
    gameState.buttonEventToProcess = params.buf[0] - '0';
  }
}

const char* WebDashboard::mapGenericGearToLocalGear(GearState inputGear) {
  switch(inputGear) {
    case GearState_Manual_1: return "1"; break;
    case GearState_Manual_2: return "2"; break;
    case GearState_Manual_3: return "3"; break;
    case GearState_Manual_4: return "4"; break;
    case GearState_Manual_5: return "5"; break;
    case GearState_Manual_6: return "6"; break;
    case GearState_Manual_7: return "7"; break;
    case GearState_Manual_8: return "8"; break;
    case GearState_Manual_9: return "9"; break;
    case GearState_Manual_10: return "10"; break;
    case GearState_Auto_P: return "P"; break;
    case GearState_Auto_R: return "R"; break;
    case GearState_Auto_N: return "N"; break;
    case GearState_Auto_D: return "D"; break;
    case GearState_Auto_S: return "S"; break;
  }
}

GearState WebDashboard::mapLocalGearToGenericGear(const char *gear) {
    if (!gear || !gear[0]) {
      return GearState_Auto_P;
    }

    if (strcmp(gear, "10") == 0) {
      return GearState_Manual_10;
    }

    switch (gear[0]) {
        case '1': return GearState_Manual_1;
        case '2': return GearState_Manual_2;
        case '3': return GearState_Manual_3;
        case '4': return GearState_Manual_4;
        case '5': return GearState_Manual_5;
        case '6': return GearState_Manual_6;
        case '7': return GearState_Manual_7;
        case '8': return GearState_Manual_8;
        case '9': return GearState_Manual_9;

        case 'P': return GearState_Auto_P;
        case 'R': return GearState_Auto_R;
        case 'N': return GearState_Auto_N;
        case 'D': return GearState_Auto_D;
        case 'S': return GearState_Auto_S;
    }

    return GearState_Auto_P;
}

const char* WebDashboard::mapGenericDriveModeToLocalDriveMode(uint8_t driveMode) {
  switch(driveMode) {
    case 1: return "Traction"; break;
    case 2: return "Comfort"; break;
    case 4: return "Sport"; break;
    case 5: return "Sport+"; break;
    case 6: return "DSC off"; break;
    case 7: return "Eco pro"; break;
  }
}

uint8_t WebDashboard::mapLocalDriveModeToGenericDriveMode(const char *driveMode) {
    if (!driveMode || !driveMode[0]) {
      return 2;
    }

    if (strcmp(driveMode, "Traction") == 0) {
      return 1;
    } else if (strcmp(driveMode, "Comfort") == 0) {
      return 2;
    } else if (strcmp(driveMode, "Sport") == 0) {
      return 4;
    } else if (strcmp(driveMode, "Sport+") == 0) {
      return 5;
    } else if (strcmp(driveMode, "DSC off") == 0) {
      return 6;
    } else if (strcmp(driveMode, "Eco pro") == 0) {
      return 7;
    } else {
      return 2;
    }
}

void WebDashboard::update() {
  // Prevent too frequent updates of the web dashboard
  if (millis() - lastWebDashboardUpdateTime >= webDashboardUpdateInterval) {
    glue_update_state();
    lastWebDashboardUpdateTime = millis();
  }
}

void WebDashboard::handleDebug(struct debug &data, MCP_CAN& CAN1, MCP_CAN* CAN2) {
  if (data.enabled == true && data.bytes > 0) {
    if (millis() - data.delay >= lastDebugUpdateInterval) {
      uint8_t frame[8] = {data.byte0, data.byte1, data.byte2, data.byte3, data.byte4, data.byte5, data.byte6, data.byte7};

      int bytes = data.bytes > 8 ? 8 : data.bytes;

     if (data.bus == 2 && CAN2 != nullptr) {
        CAN2->sendMsgBuf(data.address, 0, bytes, frame);
      } else {
        CAN1.sendMsgBuf(data.address, 0, bytes, frame);
      }

      lastDebugUpdateInterval = millis();
    }
  }
}
