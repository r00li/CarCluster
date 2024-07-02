// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "BMWFSeriesCluster.h"
#include "CRC8.h"
#include "MultiMap.h"

#include <mcp_can.h>

#define lo8(x) (uint8_t)((x) & 0xFF)
#define hi8(x) (uint8_t)(((x)>>8) & 0xFF)

uint8_t mapGenericGearToLocalGear(GearState inputGear) {
  // The gear that the car is in: 0 = clear, 1-9 = M1-M9, 10 = P, 11 = R, 12 = N, 13 = D

  switch(inputGear) {
    case GearState_Manual_1: return 1; break;
    case GearState_Manual_2: return 2; break;
    case GearState_Manual_3: return 3; break;
    case GearState_Manual_4: return 4; break;
    case GearState_Manual_5: return 5; break;
    case GearState_Manual_6: return 6; break;
    case GearState_Manual_7: return 7; break;
    case GearState_Manual_8: return 8; break;
    case GearState_Manual_9: return 9; break;
    case GearState_Manual_10: return 13; break;
    case GearState_Auto_P: return 10; break;
    case GearState_Auto_R: return 11; break;
    case GearState_Auto_N: return 12; break;
    case GearState_Auto_D: return 13; break;
    case GearState_Auto_S: return 13; break;
  }
}

int mapSpeed(GameState& game) {
  int scaledSpeed = game.speed * game.configuration.speedCorrectionFactor;
  if (scaledSpeed > game.configuration.maximumSpeedValue) {
    return game.configuration.maximumSpeedValue;
  } else {
    return scaledSpeed;
  }
}

int mapRPM(GameState& game) {
  int scaledRPM = game.rpm * game.configuration.speedCorrectionFactor;
  if (scaledRPM > game.configuration.maximumRPMValue) {
    return game.configuration.maximumRPMValue;
  } else {
    return scaledRPM;
  }
}

int mapCoolantTemperature(GameState& game) {
  if (game.coolantTemperature < game.configuration.minimumCoolantTemperature) { return game.configuration.minimumCoolantTemperature; }
  if (game.coolantTemperature > game.configuration.maximumCoolantTemperature) { return game.configuration.maximumCoolantTemperature; }
  return game.coolantTemperature;
}

BMWFSeriesCluster::BMWFSeriesCluster(MCP_CAN& CAN, bool isCarMini): CAN(CAN) {
  this->isCarMini = isCarMini;

  if (isCarMini) {
    inFuelRange[0] = 0; inFuelRange[1] = 50; inFuelRange[2] = 100;
    outFuelRange[0] = 22; inFuelRange[1] = 7; inFuelRange[2] = 3;
  } else {
    inFuelRange[0] = 0; inFuelRange[1] = 50; inFuelRange[2] = 100;
    outFuelRange[0] = 37; inFuelRange[1] = 18; inFuelRange[2] = 4;
  }
  crc8Calculator.begin();
}

void BMWFSeriesCluster::updateWithGame(GameState& game) {
  if (millis() - lastDashboardUpdateTime >= dashboardUpdateTime100) {
    // This should probably be done using a more sophisticated method like a
    // scheduler, but for now this seems to work.

    sendIgnitionStatus(game.ignition);
    sendSpeed(mapSpeed(game));
    sendRPM(mapRPM(game), mapGenericGearToLocalGear(game.gear));
    sendBasicDriveInfo(mapCoolantTemperature(game));
    sendAutomaticTransmission(mapGenericGearToLocalGear(game.gear));
    sendFuel(game.fuelQuantity, inFuelRange, outFuelRange, isCarMini);
    sendParkBrake(game.handbrake);
    sendDistanceTravelled(mapSpeed(game));
    sendAlerts(game.offroadLight, game.doorOpen, game.handbrake, isCarMini);

    counter4Bit++;
    if (counter4Bit >= 14) { counter4Bit = 0; }

    count++;
    if (count >= 254) { count = 0; } // Needs to be reset at 254 not 255

    lastDashboardUpdateTime = millis();
  }

  if (millis() - lastDashboardUpdateTime1000ms >= dashboardUpdateTime1000) {
    sendLights(game.mainLights, game.highBeam, game.rearFogLight, game.frontFogLight);
    sendBlinkers(game.leftTurningIndicator, game.rightTurningIndicator);
    sendBacklightBrightness(game.backlightBrightness);
    sendDriveMode(game.driveMode);

    sendSteeringWheelButton(game.buttonEventToProcess);
    if (game.buttonEventToProcess != 0) {
      game.buttonEventToProcess = 0;
    }

    lastDashboardUpdateTime1000ms = millis();
  }
}

void BMWFSeriesCluster::sendIgnitionStatus(boolean ignition) {
  uint8_t ignitionStatus = ignition ? 0x8A : 0x8;
  unsigned char ignitionWithoutCRC[] = { 0x80|counter4Bit, ignitionStatus, 0xDD, 0xF1, 0x01, 0x30, 0x06 };
  unsigned char ignitionWithCRC[] = { crc8Calculator.get_crc8(ignitionWithoutCRC, 7, 0x44), ignitionWithoutCRC[0], ignitionWithoutCRC[1], ignitionWithoutCRC[2], ignitionWithoutCRC[3], ignitionWithoutCRC[4], ignitionWithoutCRC[5], ignitionWithoutCRC[6] };
  CAN.sendMsgBuf(0x12F, 0, 8, ignitionWithCRC);
}

void BMWFSeriesCluster::sendSpeed(int speed) {
  uint16_t calculatedSpeed = (double)speed * 64.01;
  unsigned char speedWithoutCRC[] = { 0xC0|counter4Bit, lo8(calculatedSpeed), hi8(calculatedSpeed), (speed == 0 ? 0x81 : 0x91) };
  unsigned char speedWithCRC[] = { crc8Calculator.get_crc8(speedWithoutCRC, 4, 0xA9), speedWithoutCRC[0], speedWithoutCRC[1], speedWithoutCRC[2], speedWithoutCRC[3] };
  CAN.sendMsgBuf(0x1A1, 0, 5, speedWithCRC);
}

void BMWFSeriesCluster::sendRPM(int rpm, int manualGear) {
  // The gear that the car is in: 0 = clear, 1-9 = M1-M9, 10 = P, 11 = R, 12 = N, 13 = D
  int calculatedGear = 0;
  switch (manualGear) {
    case 0: calculatedGear = 0; break; // Empty
    case 1 ... 9: calculatedGear = manualGear + 4; break; // 1-9
    case 11: calculatedGear = 2; break; // Reverse
    case 12: calculatedGear = 1; break; // Neutral
  }
  int rpmValue =  map(rpm, 0, 6900, 0x00, 0x2B);
  unsigned char rpmWithoutCRC[] = { 0x60|counter4Bit, rpmValue, 0xC0, 0xF0, calculatedGear, 0xFF, 0xFF };
  unsigned char rpmWithCRC[] = { crc8Calculator.get_crc8(rpmWithoutCRC, 7, 0x7A), rpmWithoutCRC[0], rpmWithoutCRC[1], rpmWithoutCRC[2], rpmWithoutCRC[3], rpmWithoutCRC[4], rpmWithoutCRC[5], rpmWithoutCRC[6] };
  CAN.sendMsgBuf(0x0F3, 0, 8, rpmWithCRC);
}

void BMWFSeriesCluster::sendAutomaticTransmission(int gear) {
  // The gear that the car is in: 0 = clear, 1-9 = M1-M9, 10 = P, 11 = R, 12 = N, 13 = D
  uint8_t selectedGear = 0;
  switch (gear) {
    case 1 ... 9: selectedGear = 0x81; break; // DS
    case 10: selectedGear = 0x20; break; // P
    case 11: selectedGear = 0x40; break; // R
    case 12: selectedGear = 0x60; break; // N
    case 13: selectedGear = 0x80; break; // D
  }
  unsigned char transmissionWithoutCRC[] = { counter4Bit, selectedGear, 0xFC, 0xFF }; //0x20= P, 0x40= R, 0x60= N, 0x80= D, 0x81= DS
  unsigned char transmissionWithCRC[] = { crc8Calculator.get_crc8(transmissionWithoutCRC, 4, 0xD6), transmissionWithoutCRC[0], transmissionWithoutCRC[1], transmissionWithoutCRC[2], transmissionWithoutCRC[3] };
  CAN.sendMsgBuf(0x3FD, 0, 5, transmissionWithCRC);
}

void BMWFSeriesCluster::sendBasicDriveInfo(int engineTemperature) {
  // ABS
  unsigned char abs1WithoutCRC[] = { 0xF0|counter4Bit, 0xFE, 0xFF, 0x14 };
  unsigned char abs1WithCRC[] = { crc8Calculator.get_crc8(abs1WithoutCRC, 4, 0xD8), abs1WithoutCRC[0], abs1WithoutCRC[1], abs1WithoutCRC[2], abs1WithoutCRC[3] };
  CAN.sendMsgBuf(0x36E, 0, 5, abs1WithCRC);

  //Alive counter safety
  unsigned char aliveCounterSafetyWithoutCRC[] = { count, 0xFF };
  CAN.sendMsgBuf(0xD7, 0, 2, aliveCounterSafetyWithoutCRC);

  //Power Steering
  unsigned char steeringColumnWithoutCRC[] = { 0xF0|counter4Bit, 0xFE, 0xFF, 0x14 };
  unsigned char steeringColumnWithCRC[] = { crc8Calculator.get_crc8(steeringColumnWithoutCRC, 4, 0x9E), steeringColumnWithoutCRC[0], steeringColumnWithoutCRC[1], steeringColumnWithoutCRC[2], steeringColumnWithoutCRC[3] };
  CAN.sendMsgBuf(0x2A7, 0, 5, steeringColumnWithCRC);

  //Cruise control
  unsigned char cruiseWithoutCRC[] = { 0xF0|counter4Bit, 0xE0, 0xE0, 0xE1, 0x00, 0xEC, 0x01 };
  unsigned char cruiseWithCRC[] = { crc8Calculator.get_crc8(cruiseWithoutCRC, 7, 0x82), cruiseWithoutCRC[0], cruiseWithoutCRC[1], cruiseWithoutCRC[2], cruiseWithoutCRC[3], cruiseWithoutCRC[4], cruiseWithoutCRC[5], cruiseWithoutCRC[6] };
  CAN.sendMsgBuf(0x289, 0, 8, cruiseWithCRC);

  //Restraint system (airbag?)
  unsigned char restraintWithoutCRC[] = { 0x40|counter4Bit, 0x40, 0x55, 0xFD, 0xFF, 0xFF, 0xFF };
  unsigned char restraintWithCRC[] = { crc8Calculator.get_crc8(restraintWithoutCRC, 7, 0xFF), restraintWithoutCRC[0], restraintWithoutCRC[1], restraintWithoutCRC[2], restraintWithoutCRC[3], restraintWithoutCRC[4], restraintWithoutCRC[5], restraintWithoutCRC[6] };
  CAN.sendMsgBuf(0x19B, 0, 8, restraintWithCRC);

  //Restraint system (seatbelt?)
  unsigned char restraint2WithoutCRC[] = { 0xE0|counter4Bit, 0xF1, 0xF0, 0xF2, 0xF2, 0xFE };
  unsigned char restraint2WithCRC[] = { crc8Calculator.get_crc8(restraint2WithoutCRC, 6, 0x28), restraint2WithoutCRC[0], restraint2WithoutCRC[1], restraint2WithoutCRC[2], restraint2WithoutCRC[3], restraint2WithoutCRC[4], restraint2WithoutCRC[5] };
  CAN.sendMsgBuf(0x297, 0, 7, restraint2WithCRC);

  //TPMS
  unsigned char TPMSWithoutCRC[] = { 0xF0|counter4Bit, 0xA2, 0xA0, 0xA0 };
  unsigned char TPMSWithCRC[] = { crc8Calculator.get_crc8(TPMSWithoutCRC, 4, 0xC5), TPMSWithoutCRC[0], TPMSWithoutCRC[1], TPMSWithoutCRC[2], TPMSWithoutCRC[3] };
  CAN.sendMsgBuf(0x369, 0, 5, TPMSWithCRC);

  // Unknown (makes RPM steady)
  // Also engine temp on diesel? Range 100 - 200
  unsigned char oilWithoutCRC[] = { 0x10|counter4Bit, 0x82, 0x4E, 0x7E, engineTemperature + 50, 0x05, 0x89 };
  unsigned char oilWithCRC[] = { crc8Calculator.get_crc8(oilWithoutCRC, 7, 0xF1), oilWithoutCRC[0], oilWithoutCRC[1], oilWithoutCRC[2], oilWithoutCRC[3], oilWithoutCRC[4], oilWithoutCRC[5], oilWithoutCRC[6] };
  CAN.sendMsgBuf(0x3F9, 0, 8, oilWithCRC);

  // Engine temperature
  // range: 0 - 200
  // CRC calculation for this one is weird... there is no counter present but scans show something like CRC
  unsigned char engineTempWithoutCRC[] = { 0x3e, engineTemperature, 0x64, 0x64, 0x64, 0x01, 0xF1 };
  unsigned char engineTempWithCRC[] = { crc8Calculator.get_crc8(engineTempWithoutCRC, 7, 0xB2), engineTempWithoutCRC[0], engineTempWithoutCRC[1], engineTempWithoutCRC[2], engineTempWithoutCRC[3], engineTempWithoutCRC[4], engineTempWithoutCRC[5], engineTempWithoutCRC[6] };
  CAN.sendMsgBuf(0x2C4, 0, 8, engineTempWithCRC);
}

void BMWFSeriesCluster::sendParkBrake(boolean handbrakeActive) {
  unsigned char abs3WithoutCRC[] = { 0xF0|counter4Bit, 0x38, 0, handbrakeActive ? 0x15 : 0x14 };
  unsigned char abs3WithCRC[] = { crc8Calculator.get_crc8(abs3WithoutCRC, 4, 0x17), abs3WithoutCRC[0], abs3WithoutCRC[1], abs3WithoutCRC[2], abs3WithoutCRC[3] };
  CAN.sendMsgBuf(0x36F, 0, 5, abs3WithCRC);
}

void BMWFSeriesCluster::sendFuel(int fuelQuantity, uint8_t inFuelRange[], uint8_t outFuelRange[], boolean isCarMini) {
  //Fuel
  uint8_t fuelQuantityLiters = multiMap<uint8_t>(fuelQuantity, inFuelRange, outFuelRange, 3);
  unsigned char fuelWithoutCRC[] = { (isCarMini ? 0 : hi8(fuelQuantityLiters)), (isCarMini ? 0 : lo8(fuelQuantityLiters)), hi8(fuelQuantityLiters), lo8(fuelQuantityLiters), 0x00 };
  CAN.sendMsgBuf(0x349, 0, 5, fuelWithoutCRC);
}

void BMWFSeriesCluster::sendDistanceTravelled(int speed) {
  // MPG bar
  unsigned char mpgWithoutCRC[] = { count, 0xFF, 0x64, 0x64, 0x64, 0x01, 0xF1 };
  unsigned char mpgWithCRC[] = { crc8Calculator.get_crc8(mpgWithoutCRC, 7, 0xC6), mpgWithoutCRC[0], mpgWithoutCRC[1], mpgWithoutCRC[2], mpgWithoutCRC[3], mpgWithoutCRC[4], mpgWithoutCRC[5], mpgWithoutCRC[6] };
  CAN.sendMsgBuf(0x2C4, 0, 8, mpgWithCRC);

  // MPG bar 2 (this one actually moves the bar)
  // The distance travelled counter is used to calculate the travelled distance shown in the cluster.
  unsigned char mpg2WithoutCRC[] = { 0xF0|counter4Bit, lo8(distanceTravelledCounter), hi8(distanceTravelledCounter), 0xF2 };
  unsigned char mpg2WithCRC[] = { crc8Calculator.get_crc8(mpg2WithoutCRC, 4, 0xde), mpg2WithoutCRC[0], mpg2WithoutCRC[1], mpg2WithoutCRC[2], mpg2WithoutCRC[3], mpg2WithoutCRC[4] };
  CAN.sendMsgBuf(0x2BB, 0, 5, mpg2WithCRC);
  distanceTravelledCounter += speed*2.9;
}

void BMWFSeriesCluster::sendBlinkers(boolean leftTurningIndicator, boolean rightTurningIndicator) {
  //Blinkers
  uint8_t blinkerStatus = (leftTurningIndicator == 0 && rightTurningIndicator == 0) ? 0x80 : (0x81 | leftTurningIndicator << 4 | rightTurningIndicator << 5);
  unsigned char blinkersWithoutCRC[] = { blinkerStatus, 0xF0 };
  CAN.sendMsgBuf(0x1F6, 0, 2, blinkersWithoutCRC);
}

void BMWFSeriesCluster::sendLights(boolean mainLights, boolean highBeam, boolean rearFogLight, boolean frontFogLight) {
  //Lights
  //32 = front fog light, 64 = rear fog light, 2 = high beam, 4 = main lights
  uint8_t lightStatus = highBeam << 1 | mainLights << 2 | frontFogLight << 5 | rearFogLight << 6;
  unsigned char lightsWithoutCRC[] = { lightStatus, 0xC0, 0xF7 };
  CAN.sendMsgBuf(0x21A, 0, 3, lightsWithoutCRC);
}

void BMWFSeriesCluster::sendBacklightBrightness(uint8_t brightness) {
  // Backlight brightness
  uint8_t mappedBrightness = map(brightness, 0, 100, 0, 253);
  unsigned char backlightBrightnessWithoutCRC[] = { mappedBrightness, 0xFF };
  CAN.sendMsgBuf(0x202, 0, 2, backlightBrightnessWithoutCRC);
}

void BMWFSeriesCluster::sendAlerts(boolean offroad, boolean doorOpen, boolean handbrake, boolean isCarMini) {
  // Check control messages
  // These are complicated since the same CAN ID is used to show variety of messages
  // Sending 0x29 on byte 4 sets the alert for the ID in byte 2, while sending 0x28 clears that message
  // Known IDs:
  // 34: check engine
  // 35, 215: DSC
  // 36: DSC OFF
  // 24: Park brake error (yellow)
  // 71: Park brake error (red)
  // 77 Seat belt indicator
  if (doorOpen) {
    uint8_t message[] = { 0x40, 0x0F, 0x00, 0x29, 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
  } else {
    uint8_t message[] = { 0x40, 0x0F, 0x00, 0x28, 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
  }

  if (offroad) {
    uint8_t message[] = { 0x40, 215, 0x00, 0x29, 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
  } else {
    uint8_t message[] = { 0x40, 215, 0x00, 0x28, 0xFF, 0xFF, 0xFF, 0xFF };
    CAN.sendMsgBuf(0x5c0, 0, 8, message);
  }

  if (isCarMini) {
    if (handbrake) {
      uint8_t message[] = { 0x40, 71, 0x00, 0x29, 0xFF, 0xFF, 0xFF, 0xFF };
      CAN.sendMsgBuf(0x5c0, 0, 8, message);
    } else {
      uint8_t message[] = { 0x40, 71, 0x00, 0x28, 0xFF, 0xFF, 0xFF, 0xFF };
      CAN.sendMsgBuf(0x5c0, 0, 8, message);
    }
  }
}

void BMWFSeriesCluster::sendSteeringWheelButton(int buttonEvent) {
  //Menu buttons
  uint8_t buttonMessage[] = { 0x00, 0xFF };
  if (buttonEvent == 1) {
    buttonMessage[0] = 76;
  }
  CAN.sendMsgBuf(0x1EE, 0, 2, buttonMessage);
}

void BMWFSeriesCluster::updateLanguageAndUnits() {
  //language and units
  //CanSend(0x291, 2, 18, 0, 0x00, 0x00, 0x00, 0x00, 0x00);
  //Byte:1 language
  //Byte 2: Clock and Temp layout,format eg celius,fainhieght and am,pm
  //Byte 3: miles or km
  //Above is english, mpg
  //CanSend(0x291, 2, 18, 1, 0x00, 0x00, 0x00, 0x00, 0x00);
  //Aboove is english, l/100km
}

void BMWFSeriesCluster::sendDriveMode(uint8_t driveMode) {
  //1= Traction, 2= Comfort, 4= Sport, 5= Sport+, 6= DSC off, 7= Eco pro 
  unsigned char modeWithoutCRC[] = { 0xF0|counter4Bit, 0, 0, driveMode, 0x11, 0xC0 };
  unsigned char modeWithCRC[] = { crc8Calculator.get_crc8(modeWithoutCRC, 6, 0x4a), modeWithoutCRC[0], modeWithoutCRC[1], modeWithoutCRC[2], modeWithoutCRC[3], modeWithoutCRC[4], modeWithoutCRC[5] };
  CAN.sendMsgBuf(0x3A7, 0, 7, modeWithCRC);
}