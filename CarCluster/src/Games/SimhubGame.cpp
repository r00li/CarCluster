// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "SimhubGame.h"

SimhubGame::SimhubGame(GameState& game): Game(game) {
}

void SimhubGame::begin() {
}

void SimhubGame::decodeSerialData(JsonDocument& doc) {
  gameState.rpm = doc["rpm"];
  int max_rpm = doc["mrp"];

  const char* simGear = doc["gea"];
  switch(simGear[0]) {
    case '1': gameState.gear = GearState_Manual_1; break;
    case '2': gameState.gear = GearState_Manual_2; break;
    case '3': gameState.gear = GearState_Manual_3; break;
    case '4': gameState.gear = GearState_Manual_4; break;
    case '5': gameState.gear = GearState_Manual_5; break;
    case '6': gameState.gear = GearState_Manual_6; break;
    case '7': gameState.gear = GearState_Manual_7; break;
    case '8': gameState.gear = GearState_Manual_8; break;
    case '9': gameState.gear = GearState_Manual_9; break;
    case 'P': gameState.gear = GearState_Auto_P; break;
    case 'R': gameState.gear = GearState_Auto_R; break;
    case 'N': gameState.gear = GearState_Auto_N; break;
    case 'D': gameState.gear = GearState_Auto_D; break;
  }

  gameState.speed = doc["spe"];  
  gameState.leftTurningIndicator = doc["lft"];
  gameState.rightTurningIndicator = doc["rit"];
  gameState.coolantTemperature = doc["oit"];
  gameState.doorOpen = (doc["pau"] != 0 || doc["run"] == 0);
  gameState.fuelQuantity = doc["fue"];
  gameState.handbrake = doc["hnb"];
  gameState.absLight = doc["abs"];
  gameState.offroadLight = doc["tra"];
}