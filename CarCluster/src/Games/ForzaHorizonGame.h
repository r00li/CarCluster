// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef FORZA_HORIZON_GAME
#define FORZA_HORIZON_GAME

#include "Arduino.h"
#include "AsyncUDP.h" // For game integration (system library part of ESP core)

#include "GameSimulation.h"

class ForzaHorizonGame: public Game {
  public:
    ForzaHorizonGame(GameState& game, int port);
    void begin();

  private:
    int port;
    AsyncUDP forzaUdp;
};

#endif