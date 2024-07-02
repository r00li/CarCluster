// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "ForzaHorizonGame.h"

ForzaHorizonGame::ForzaHorizonGame(GameState& game, int port): Game(game) {
  this->port = port;
}

void ForzaHorizonGame::begin() {
  // Forza (Horizon) sends data as a UDP blob of data
  // Telemetry protocol described here: https://medium.com/@makvoid/building-a-digital-dashboard-for-forza-using-python-62a0358cb43b

  if (forzaUdp.listen(port)) {
    forzaUdp.onPacket([this](AsyncUDPPacket packet) {
      if (packet.length() == 324) {
        char dataBuff[4];  // four bytes in a float 32

        // CURRENT_ENGINE_RPM
        memcpy(dataBuff, (packet.data() + 16), 4);
        gameState.rpm = *((float*)dataBuff);

        // IDLE_ENGINE_RPM
        //memcpy(dataBuff, (packet.data() + 12), 4);
        //if (idle_rpm != *((float*)dataBuff)) {
        //  idle_rpm = *((float*)dataBuff);
        //}

        // MAX_ENGINE_RPM
        memcpy(dataBuff, (packet.data() + 8), 4);
        float max_rpm = *((float*)dataBuff);

        if (max_rpm == 0) {
          gameState.doorOpen = true;  // We are in a menu
        } else {
          gameState.doorOpen = false;
        }

        if (max_rpm > gameState.configuration.maximumRPMValue) {
          gameState.rpm = map(gameState.rpm, 0, max_rpm, 0, gameState.configuration.maximumRPMValue);
        }

        // SPEED
        memcpy(dataBuff, (packet.data() + 256), 4);
        int someSpeed = *((float*)dataBuff);
        someSpeed = someSpeed * 3.6;
        gameState.speed = someSpeed;

        // GEAR
        memcpy(dataBuff, (packet.data() + 319), 1);
        int forzaGear = (int)(dataBuff[0]);
        if (forzaGear == 0) {
          gameState.gear = GearState_Auto_R;
        } else if (forzaGear > 10) {
          gameState.gear = GearState_Auto_D;
        } else {
          gameState.gear = static_cast<GearState>(forzaGear);
        }
        if (max_rpm == 0) { 
          gameState.gear = GearState_Auto_P; // Idle
        }

        // HANDBRAKE
        memcpy(dataBuff, (packet.data() + 318), 1);
        int handbrake = (int)(dataBuff[0]);
        gameState.handbrake = handbrake > 0 ? true : false;
      }
    });
  }
}