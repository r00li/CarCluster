// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef BEAM_NG_GAME
#define BEAM_NG_GAME

#include "Arduino.h"
#include "AsyncUDP.h" // For game integration (system library part of ESP core)

#include "GameSimulation.h"

class BeamNGGame: public Game {
  public:
    BeamNGGame(GameState& game, int port);
    void begin();

  private:
    int port;
    AsyncUDP beamUdp;
};

#endif