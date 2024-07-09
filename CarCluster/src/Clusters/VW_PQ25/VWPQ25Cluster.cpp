// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "VWPQ25Cluster.h"

// Helper constants
#define lo8(x) ((int)(x)&0xff)
#define hi8(x) ((int)(x) >> 8)

VWPQ25Cluster::VWPQ25Cluster(MCP_CAN& CAN, int fuelPotIncPin, int fuelPotDirPin, int fuelPot1CsPin, int fuelPot2CsPin, int sprinklerWaterSensor, int coolantShortageSensor, int oilPressureSwitch, int handbrakeIndicator, int brakeFluidWarning): CAN(CAN) {
  fuelPot.begin(fuelPotIncPin, fuelPotDirPin, fuelPot1CsPin);
  fuelPot.setPosition(100, true); // Force the pot to a known value

  fuelPot2.begin(fuelPotIncPin, fuelPotDirPin, fuelPot2CsPin);
  fuelPot2.setPosition(100, true); // Force the pot to a known value

  this->sprinklerWaterSensor = sprinklerWaterSensor;
  this->coolantShortageSensor = coolantShortageSensor;
  this->oilPressureSwitch = oilPressureSwitch;
  this->handbrakeIndicator = handbrakeIndicator;
  this->brakeFluidWarning = brakeFluidWarning;

  pinMode(sprinklerWaterSensor, OUTPUT);
  pinMode(coolantShortageSensor, OUTPUT);
  pinMode(oilPressureSwitch, OUTPUT);
  pinMode(handbrakeIndicator, OUTPUT);
  pinMode(brakeFluidWarning, OUTPUT);

  digitalWrite(sprinklerWaterSensor, HIGH);
  digitalWrite(coolantShortageSensor, HIGH);
  digitalWrite(oilPressureSwitch, LOW);
  digitalWrite(handbrakeIndicator, HIGH);
  digitalWrite(brakeFluidWarning, LOW);
}

void VWPQ25Cluster::setFuel(GameState& game) {
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

uint8_t VWPQ25Cluster::mapGenericGearToLocalGear(GearState inputGear) {
  // The gear that the car is in: 0 = clear, 1-9 = M1-M9, 10 = P, 11 = R, 12 = N, 13 = D

  switch(inputGear) {
    case GearState_Manual_1: return 1; break;
    case GearState_Manual_2: return 2; break;
    case GearState_Manual_3: return 3; break;
    case GearState_Manual_4: return 4; break;
    case GearState_Manual_5: return 5; break;
    case GearState_Manual_6: return 6; break;
    case GearState_Manual_7: return 7; break;
    case GearState_Manual_8: return 10; break;
    case GearState_Manual_9: return 10; break;
    case GearState_Manual_10: return 10; break;
    case GearState_Auto_P: return 0; break;
    case GearState_Auto_R: return 8; break;
    case GearState_Auto_N: return 9; break;
    case GearState_Auto_D: return 10; break;
    case GearState_Auto_S: return 10; break;
  }
}

int VWPQ25Cluster::mapSpeed(GameState& game) {
  int scaledSpeed = game.speed * game.configuration.speedCorrectionFactor;
  if (scaledSpeed > game.configuration.maximumSpeedValue) {
    return game.configuration.maximumSpeedValue;
  } else {
    return scaledSpeed;
  }
}

int VWPQ25Cluster::mapRPM(GameState& game) {
  int scaledRPM = game.rpm * game.configuration.speedCorrectionFactor;
  if (scaledRPM > game.configuration.maximumRPMValue) {
    return game.configuration.maximumRPMValue;
  } else {
    return scaledRPM;
  }
}

int VWPQ25Cluster::mapCoolantTemperature(GameState& game) {
  if (game.coolantTemperature < game.configuration.minimumCoolantTemperature) { return game.configuration.minimumCoolantTemperature; }
  if (game.coolantTemperature > game.configuration.maximumCoolantTemperature) { return game.configuration.maximumCoolantTemperature; }
  return game.coolantTemperature;
}

void VWPQ25Cluster::updateWithGame(GameState& game) {  if (millis() - lastDashboardUpdateTime >= dashboardUpdateTime50) {
    // This should probably be done using a more sophisticated method like a
    // scheduler, but for now this seems to work.

    sendImmobilizer();
    sendIndicators(game.leftTurningIndicator, game.rightTurningIndicator, game.turningIndicatorsBlinking, game.highBeam, game.frontFogLight, false, false, game.doorOpen, game.backlightBrightness);
    sendDieselEngine();
    sendRPM(mapRPM(game));
    sendSpeed(mapSpeed(game), false, game.offroadLight, game.absLight);
    sendABS(mapSpeed(game));
    sendGear(mapGenericGearToLocalGear(game.gear));
    sendAirbag(false);

    seq++;
    if (seq > 15) {
      seq = 0;
    }

    lastDashboardUpdateTime = millis();
  }

  if (millis() - lastDashboardUpdateTime500ms >= dashboardUpdateTime500) {
    setFuel(game);
    
    // Manipulation with digital I/O
    digitalWrite(oilPressureSwitch, mapRPM(game) > 1500 ? LOW : HIGH);
    digitalWrite(handbrakeIndicator, game.handbrake ? LOW : HIGH);
    
    lastDashboardUpdateTime500ms = millis();
  }
}

void VWPQ25Cluster::sendImmobilizer() {
  CAN.sendMsgBuf(IMMOBILIZER_ID, 0, 8, immobilizerBuffer);
}

void VWPQ25Cluster::sendIndicators(boolean leftBlinker, boolean rightBlinker, boolean blinkersBlinking, boolean highBeam, boolean frontFogLight, boolean batteryWarning, boolean trunkOpen, boolean doorOpen, int brightness) {
  uint8_t temp_turning_lights = 0 | leftBlinker | (rightBlinker << 1);
  if (blinkersBlinking == true) {
    turning_lights_counter = turning_lights_counter + 1;
    if (turning_lights_counter <= 8) {
    } else if (turning_lights_counter > 8 && turning_lights_counter < 16) {
      temp_turning_lights = 0;
    } else {
      turning_lights_counter = 0;
    }
  }

  int temp_battery_warning = batteryWarning ? B10000000 : 0;
  int temp_trunklid_open = trunkOpen ? B00100000 : 0;
  int temp_check_lamp = 0; // B00010000 - check if this works
  int temp_clutch_control = 0; // B00000001 - check if this works
  int temp_keybattery_warning = 0; // B10000000 - check if this works
  int lightmode = (highBeam << 6) | (frontFogLight << 5);

  indicatorsBuffer[0] = temp_battery_warning + temp_turning_lights;
  indicatorsBuffer[1] = temp_trunklid_open + doorOpen;
  indicatorsBuffer[2] = brightness;
  indicatorsBuffer[4] = temp_check_lamp + temp_clutch_control;
  indicatorsBuffer[5] = temp_keybattery_warning;
  indicatorsBuffer[7] = lightmode;
  CAN.sendMsgBuf(INDICATORS_ID, 0, 8, indicatorsBuffer);
}

void VWPQ25Cluster::sendDieselEngine() {
  CAN.sendMsgBuf(DIESEL_ENGINE_ID, 0, 8, diselEngineBuffer);
}

void VWPQ25Cluster::sendRPM(int rpm) {
  rpmBuffer[2] = (uint8_t)((rpm * 4) & 0x00FF);
  rpmBuffer[3] = (uint8_t)((rpm * 4) >> 8);

  CAN.sendMsgBuf(RPM_ID, 0, 8, rpmBuffer);
}

void VWPQ25Cluster::sendSpeed(int speed, boolean tpmsLight, boolean espLight, boolean absLight) {
  int wheelSpeed = speed * 146;
  distanceCounter += speed * dashboardUpdateTime50 / 182;
  if (distanceCounter > 30000) distanceCounter -= 30000;

  speedBuffer[1] = lo8(wheelSpeed);
  speedBuffer[2] = hi8(wheelSpeed);
  speedBuffer[3] = (tpmsLight << 3) | (espLight << 1) | absLight;
  speedBuffer[5] = (uint8_t)(distanceCounter & 0x00FF);   // Distance low byte
  speedBuffer[6] = (uint8_t)(distanceCounter >> 8);       // Distance high byte

  CAN.sendMsgBuf(SPEED_ID, 0, 8, speedBuffer);
}

void VWPQ25Cluster::sendABS(int speed) {
  absBuffer[2] = lo8(speed);
  absBuffer[3] = hi8(speed);
  absBuffer[7] = (0x10 | seq);

  CAN.sendMsgBuf(ABS_ID, 0, 8, absBuffer);
}

void VWPQ25Cluster::sendGear(uint8_t gear) {
  uint8_t tempGear = gear;
  switch (tempGear) {
    case 0: tempGear = 0; break;                                     // P
    case 1 ... 7: tempGear = 0x2E + ((tempGear - 1) * 0x10); break;  // 1-7
    case 8: tempGear = 0x36; break;                                  // R
    case 9: tempGear = 0x40; break;                                  // N
    case 10: tempGear = 0x56; break;                                 // D
  }
  // Last byte 56=D, 36=R, 40=N, 2E=1, 3E=2, 4E=3, 5E=4 6E=5, 7E=6, 8E=7
  gearShifterBuffer[7] = tempGear;

  CAN.sendMsgBuf(GEAR_ID, 0, 8, gearShifterBuffer);
}

void VWPQ25Cluster::sendAirbag(boolean seatBeltLight) {
  airbagBuffer[2] = (seatBeltLight << 2);

  CAN.sendMsgBuf(AIRBAG_ID, 0, 8, airbagBuffer);
}