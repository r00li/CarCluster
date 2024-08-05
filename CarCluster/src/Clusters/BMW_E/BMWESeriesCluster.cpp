// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "BMWESeriesCluster.h"

BMWESeriesCluster::BMWESeriesCluster(MCP_CAN& CAN, int fuelPotIncPin, int fuelPotDirPin, int fuelPot1CsPin, int fuelPot2CsPin, int handbrakeIndicatorPin): CAN(CAN) {
  fuelPot.begin(fuelPotIncPin, fuelPotDirPin, fuelPot1CsPin);
  fuelPot.setPosition(100, true); // Force the pot to a known value

  fuelPot2.begin(fuelPotIncPin, fuelPotDirPin, fuelPot2CsPin);
  fuelPot2.setPosition(100, true); // Force the pot to a known value

  this->handbrakeIndicator = handbrakeIndicatorPin;

  // To get HIGH Z output
  pinMode(handbrakeIndicatorPin, INPUT);

  digitalWrite(handbrakeIndicatorPin, HIGH);
}

void BMWESeriesCluster::setFuel(GameState& game) {
  int percentage = game.fuelQuantity;
  if (game.configuration.isDualFuelPot) {
    int pot1Percentage = (percentage > 50) ? percentage - 50 : 0;
    int pot2Percentage = (percentage < 50) ? percentage : 50;

    int desiredPositionPot1 = map(pot1Percentage, 0, 50, game.configuration.minimumFuelPotValue, game.configuration.maximumFuelPotValue);
    fuelPot.setPosition(desiredPositionPot1, false);
  
    int desiredPositionPot2 = map(pot2Percentage, 0, 50, game.configuration.minimumFuelPot2Value, game.configuration.maximumFuelPot2Value);
    fuelPot2.setPosition(desiredPositionPot2, false);
  } else {
    int desiredPosition = map(percentage, 0, 100, game.configuration.minimumFuelPotValue, game.configuration.maximumFuelPotValue);
    fuelPot.setPosition(desiredPosition, false);
  }
}

uint8_t BMWESeriesCluster::mapGenericGearToLocalGear(GearState inputGear) {
  // The gear that the car is in: 0 = clear, 1-9 = M1-M9, 10 = P, 11 = R, 12 = N, 13 = D

  switch(inputGear) {
    case GearState_Manual_1: return 0x78; break;
    case GearState_Manual_2: return 0x78; break;
    case GearState_Manual_3: return 0x78; break;
    case GearState_Manual_4: return 0x78; break;
    case GearState_Manual_5: return 0x78; break;
    case GearState_Manual_6: return 0x78; break;
    case GearState_Manual_7: return 0x78; break;
    case GearState_Manual_8: return 0x78; break;
    case GearState_Manual_9: return 0x78; break;
    case GearState_Manual_10: return 0x78; break;
    case GearState_Auto_P: return 0xE1; break;
    case GearState_Auto_R: return 0xD2; break;
    case GearState_Auto_N: return 0x87; break;
    case GearState_Auto_D: return 0x78; break;
    case GearState_Auto_S: return 0x78; break;
  }
}

int BMWESeriesCluster::mapSpeed(GameState& game) {
  int scaledSpeed = game.speed * game.configuration.speedCorrectionFactor;
  if (scaledSpeed > game.configuration.maximumSpeedValue) {
    return game.configuration.maximumSpeedValue;
  } else {
    return scaledSpeed;
  }
}

int BMWESeriesCluster::mapRPM(GameState& game) {
  int scaledRPM = game.rpm * game.configuration.speedCorrectionFactor;
  if (scaledRPM > game.configuration.maximumRPMValue) {
    return game.configuration.maximumRPMValue;
  } else {
    return scaledRPM;
  }
}

int BMWESeriesCluster::mapCoolantTemperature(GameState& game) {
  if (game.coolantTemperature < game.configuration.minimumCoolantTemperature) { return game.configuration.minimumCoolantTemperature; }
  if (game.coolantTemperature > game.configuration.maximumCoolantTemperature) { return game.configuration.maximumCoolantTemperature; }
  return game.coolantTemperature;
}

void BMWESeriesCluster::updateWithGame(GameState& game) {
  if (millis() - lastDashboardUpdateTime >= dashboardUpdateTimeShort) {
    // This should probably be done using a more sophisticated method like a
    // scheduler, but for now this seems to work.

    sendIgnition(game.ignition);
    sendRPM(mapRPM(game));
    sendSpeed(mapSpeed(game), dashboardUpdateTimeShort);
    sendSteeringWheel();

    sendAirbagCounter(); // 200ms
    sendABSCounter(); // 200ms
    sendABS(game.rpm, game.speed); // 200ms
    sendGearStatus(mapGenericGearToLocalGear(game.gear)); // 200ms

    counter++;

    lastDashboardUpdateTime = millis();
  }

  if (millis() - lastDashboardUpdateTimeLong >= dashboardUpdateTimeLong) {
    sendLights(game.mainLights, game.highBeam, game.frontFogLight, game.rearFogLight);
    sendEngineTemperature(mapCoolantTemperature(game));
    sendHanbrake(game.handbrake);
    sendBacklightBrightness(game.backlightBrightness);
    sendBlinkers(game.leftTurningIndicator, game.rightTurningIndicator);

    sendSteeringWheelControls(game.buttonEventToProcess);
    game.buttonEventToProcess = 0;

    //setFuel(game);
    
    lastDashboardUpdateTimeLong = millis();
  }
}

void BMWESeriesCluster::sendIgnition(bool ignition) {
  uint8_t ignitionFrame[5] = { (ignition ? 0x45 : 0x00), (ignition ? 0x42 : 0x00), (ignition ? 0x69 : 00), (ignition ? 0x8f : 0x00), counter };
  CAN.sendMsgBuf(0x130, 0, 5, ignitionFrame);
}

void BMWESeriesCluster::sendRPM(int rpm) {
  uint16_t value = rpm * 4;
  uint8_t rpmFrame[8] = {0x5F, 0x59, 0xFF, 0x00, value, (value >> 8), 0x80, 0x99};

  CAN.sendMsgBuf(0x0AA, 0, 8, rpmFrame);
}

void BMWESeriesCluster::sendSpeed(int speed, int deltaTime) {
  uint16_t speed_value = speed + previousSpeedValue;

  uint16_t counter = (speedFrame[6] | (speedFrame[7] << 8)) & 0x0FFF;
  counter += (float)deltaTime * M_PI;

  speedFrame[0] = speed_value;
  speedFrame[1] = (speed_value >> 8);

  speedFrame[2] = speedFrame[0];
  speedFrame[3] = speedFrame[1];

  speedFrame[4] = speedFrame[0];
  speedFrame[5] = speedFrame[1];

  speedFrame[6] = counter;
  speedFrame[7] = (counter >> 8) | 0xF0;

  CAN.sendMsgBuf(0x1A6, 0, 8, speedFrame);
  previousSpeedValue = speed_value;
}

void BMWESeriesCluster::sendSteeringWheel() {
  uint8_t steeringWheelFrame[7] = {0x83, 0xFD, 0xFC, 0x00, 0x00, 0xFF, 0xF1};

  steeringWheelFrame[1] = 0;
  steeringWheelFrame[2] = 0;

  CAN.sendMsgBuf(0x0C4, 0, 7, steeringWheelFrame);
}

void BMWESeriesCluster::sendAirbagCounter() {
  uint8_t airbagFrame[2] = {counter, 0xFF};

  CAN.sendMsgBuf(0x0D7, 0, 2, airbagFrame);
}

void BMWESeriesCluster::sendABSCounter() {
  uint8_t absCounterFrame[2] = {counter | 0xF0, 0xFF};

  CAN.sendMsgBuf(0x0C0, 0, 2, absCounterFrame);
}

void BMWESeriesCluster::sendABS(uint8_t a1, uint8_t a2) {
  absFrame[2] = ((((absFrame[2] >> 4) + 3) << 4) & 0xF0) | 0x03;
  absFrame[0] = random(0,255);
  absFrame[1] = random(0,255);
  absFrame[3] = random(0,255);
  absFrame[4] = random(0,255);
  absFrame[5] = random(0,255);
  absFrame[6] = random(0,255);
  absFrame[7] = random(0,255);
  CAN.sendMsgBuf(0x19E, 0, 8, absFrame);
}

void BMWESeriesCluster::sendLights(bool mainLights, bool highBeam, bool frontFogLight, bool rearFogLight) {
  uint8_t lightFrame[3] = {0x00, 0x12, 0xf7};

  uint8_t lights = ((mainLights || highBeam || frontFogLight || rearFogLight) << 0) | (highBeam << 1) | (mainLights << 2) | (frontFogLight << 5) | (rearFogLight << 6);
  lightFrame[0] = lights;

  CAN.sendMsgBuf(0x21A, 0, 3, lightFrame);
}

void BMWESeriesCluster::sendEngineTemperature(int coolantTemperature) {
  uint8_t engineTempFrame[8] = {0x8B, 0xFF, 0x63, 0xCD, 0x5D, 0x37, 0xCD, 0xA8};

  engineTempFrame[0] = coolantTemperature + 48;
  engineTempFrame[2] = counter;

  CAN.sendMsgBuf(0x1D0, 0, 8, engineTempFrame);
}

// For clusters with park brake via CAN
void BMWESeriesCluster::sendHanbrake(bool handbrake) {
  uint8_t handbrakeFrame[2] = {handbrake ? 0xFE : 0xFD, 0xFF};
  CAN.sendMsgBuf(0x34F, 0, 2, handbrakeFrame);

  if (handbrake) {
    pinMode(handbrakeIndicator, OUTPUT);
    digitalWrite(handbrakeIndicator, LOW);
  } else {
    pinMode(handbrakeIndicator, INPUT);
  }
}

void BMWESeriesCluster::sendSteeringWheelControls(int button) {
  uint8_t buttonFrame[2] = {0x00, 0xFF};

  switch (button) {
    case 1: buttonFrame[0] = 0x40; break;
    case 2: buttonFrame[0] = 0x80; break;
    case 3: sendTime(); break; // Abusing the system a bit to send the time using button 3
  }

  CAN.sendMsgBuf(0x1EE, 0, 2, buttonFrame);
}

void BMWESeriesCluster::sendGearStatus(int gear) {
  // 0xE1, C3 = P
  // 0xD2 = R
  // 0xF0 = Clear
  // 0x78 = D
  // 0x87, 0x96 = N
  uint8_t gearFrame[6] = {gear, 0x0C, 0x80, (counterGear << 4 | 0x0C), 0xF0, 0xFF};
  CAN.sendMsgBuf(0x1D2, 0, 6, gearFrame);

  counterGear++;
  if (counterGear > 15) {
    counterGear = 0;
  }
}

void BMWESeriesCluster::sendBacklightBrightness(int brightness) {
  uint8_t mappedBrightness = map(brightness, 1, 100, 1, 0xFD);
  uint8_t brightnessFrame[2] = {brightness == 0 ? 0xFE : mappedBrightness, 0xFF};
  CAN.sendMsgBuf(0x202, 0, 2, brightnessFrame);
}

void BMWESeriesCluster::sendBlinkers(bool leftBlinker, bool rightBlinker) {
  uint8_t blinkerFrame[2] = {0x80, 0xF0};
  blinkerFrame[0] = blinkerFrame[0] | (leftBlinker || rightBlinker) | (leftBlinker << 4) | (rightBlinker << 5);
  CAN.sendMsgBuf(0x1F6, 0, 2, blinkerFrame);
}

void BMWESeriesCluster::sendTime() {
  uint8_t timeFrame[8] = {0x0B, 0x10, 0x00, 0x0D, 0x1F, 0xDF, 0x07, 0xF2};

  timeFrame[0] = 12; // Hour
  timeFrame[1] = 0; // Minute
  timeFrame[2] = 0; // Second

  timeFrame[3] = 1; // Day
  timeFrame[4] = (8 << 4) | 0x0F; // Month

  uint16_t year = 2024;
  timeFrame[5] = (uint8_t)year;
  timeFrame[6] = (uint8_t)(year >> 8);
  CAN.sendMsgBuf(0x39E, 0, 8, timeFrame);
}
