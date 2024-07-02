// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "BeamNGGame.h"

BeamNGGame::BeamNGGame(GameState& game, int port): Game(game) {
  this->port = port;
}

void BeamNGGame::begin() {
  // Beam NG sends data as a UDP blob of data
  // Telemetry protocol described here: https://github.com/fuelsoft/out-gauge-cluster

  if (beamUdp.listen(port)) {
    beamUdp.onPacket([this](AsyncUDPPacket packet) {
      if (packet.length() >= 64) {
        char dataBuff[4];  // four bytes

        // GEAR
        memcpy(dataBuff, (packet.data() + 10), 1);
        int beamGear = (int)(0xFF & dataBuff[0]);
        if (beamGear == 0) {
          gameState.gear = GearState_Auto_R;
        } else if (beamGear == 1) {
          gameState.gear = GearState_Auto_D;
        } else if (beamGear >= 10) {
          gameState.gear = GearState_Auto_D;
        } else {
          gameState.gear = static_cast<GearState>(beamGear - 1);
        }

        // SPEED
        memcpy(dataBuff, (packet.data() + 12), 4);
        int someSpeed = *((float*)dataBuff);
        someSpeed = someSpeed * 3.6;               // Speed is in m/s
        gameState.speed = someSpeed;

        // CURRENT_ENGINE_RPM
        memcpy(dataBuff, (packet.data() + 16), 4);
        gameState.rpm = *((float*)dataBuff);

        // ENGINE TEMPERATURE
        memcpy(dataBuff, (packet.data() + 24), 4);
        gameState.coolantTemperature = *((float*)dataBuff);

        // LIGHTS
        memcpy(dataBuff, (packet.data() + 44), 4);
        int lights = *((int*)dataBuff);

        gameState.rightTurningIndicator = ((lights & 0x0040) != 0);
        gameState.leftTurningIndicator = ((lights & 0x0020) != 0);
        gameState.highBeam = ((lights & 0x0002) != 0);
        gameState.batteryLight = ((lights & 0x0200) != 0);
        gameState.absLight = ((lights & 0x0400) != 0);
        gameState.handbrake = ((lights & 0x0004) != 0);
        gameState.offroadLight = ((lights & 0x0010) != 0);
      }
    });
  }
}