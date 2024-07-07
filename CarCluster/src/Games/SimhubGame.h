// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef SIMHUB_GAME
#define SIMHUB_GAME

#include "Arduino.h"
#include "../Libs/ArduinoJson/ArduinoJson.h"  // For parsing serial data and for ESPDash ( https://github.com/bblanchon/ArduinoJson )

#include "GameSimulation.h"

class SimhubGame: public Game {
  public:
    SimhubGame(GameState& game);
    void begin();
    void decodeSerialData(JsonDocument& doc);
};

#endif