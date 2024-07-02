// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef SIMHUB_GAME
#define SIMHUB_GAME

#include <Arduino.h>
#include <ArduinoJson.h>  // For parsing serial data and for ESPDash (install through library manager: ArduinoJson by Benoit Blanchon - tested using 6.20.1)
#include "GameSimulation.h"

class SimhubGame: public Game {
  public:
    SimhubGame(GameState& game);
    void begin();
    void decodeSerialData(JsonDocument& doc);
};

#endif