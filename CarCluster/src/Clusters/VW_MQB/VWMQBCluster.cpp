// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "VWMQBCluster.h"
#include "MQBCRC.h"

VWMQBCluster::VWMQBCluster(MCP_CAN& CAN, int fuelPotIncPin, int fuelPotDirPin, int fuelPot1CsPin, int fuelPot2CsPin, bool passthroughMode): CAN(CAN) {
  fuelPot.begin(fuelPotIncPin, fuelPotDirPin, fuelPot1CsPin);
  fuelPot.setPosition(100, true); // Force the pot to a known value

  fuelPot2.begin(fuelPotIncPin, fuelPotDirPin, fuelPot2CsPin);
  fuelPot2.setPosition(100, true); // Force the pot to a known value

  this->passthroughMode = passthroughMode;
}

void VWMQBCluster::setFuel(GameState& game) {
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

uint8_t VWMQBCluster::mapGenericGearToLocalGear(GearState inputGear) {
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

int VWMQBCluster::mapSpeed(GameState& game) {
  int scaledSpeed = game.speed * game.configuration.speedCorrectionFactor;
  if (scaledSpeed > game.configuration.maximumSpeedValue) {
    return game.configuration.maximumSpeedValue;
  } else {
    return scaledSpeed;
  }
}

int VWMQBCluster::mapRPM(GameState& game) {
  int scaledRPM = game.rpm * game.configuration.speedCorrectionFactor;
  if (scaledRPM > game.configuration.maximumRPMValue) {
    return game.configuration.maximumRPMValue;
  } else {
    return scaledRPM;
  }
}

int VWMQBCluster::mapCoolantTemperature(GameState& game) {
  if (game.coolantTemperature < game.configuration.minimumCoolantTemperature) { return game.configuration.minimumCoolantTemperature; }
  if (game.coolantTemperature > game.configuration.maximumCoolantTemperature) { return game.configuration.maximumCoolantTemperature; }
  return game.coolantTemperature;
}

void VWMQBCluster::updateWithGame(GameState& game) {
  if (millis() - lastDashboardUpdateTime >= dashboardUpdateTime50) {
    // This should probably be done using a more sophisticated method like a
    // scheduler, but for now this seems to work.

    if (!passthroughMode) {
      sendIgnitionStatus(game.ignition);
      sendBacklightBrightness(game.backlightBrightness);
    }
    sendESP20();
    sendESP21(mapSpeed(game));
    sendTSK07();
    sendLhEPS01();
    sendMotor(mapRPM(game), mapCoolantTemperature(game), game.oilTemperature);
    sendESP24();
    sendGear(mapGenericGearToLocalGear(game.gear));
    sendAirbag01();
    if (!passthroughMode) {
      sendBlinkers(game.leftTurningIndicator, game.rightTurningIndicator, game.turningIndicatorsBlinking);
    }
    sendParkBrake(game.handbrake);
    //sendSWA01();

    // Testing only. To be removed
    // sendTestBuffers();

    seq++;
    if (seq > 15) {
      seq = 0;
    }

    lastDashboardUpdateTime = millis();
  }

  if (millis() - lastDashboardUpdateTime500ms >= dashboardUpdateTime500) {
    sendTPMS();
    sendLights(game.highBeam, game.rearFogLight);
    sendOtherLights();
    sendDoorStatus(game.doorOpen);
    sendOutdoorTemperature(game.outdoorTemperature);

    if (game.buttonEventToProcess != 0) {
      sendSteeringWheelControls(game.buttonEventToProcess + 3);
      game.buttonEventToProcess = 0;
    }

    setFuel(game);
    
    lastDashboardUpdateTime500ms = millis();
  }
}

void VWMQBCluster::sendIgnitionStatus(boolean ignition) {
  kStatusBuf[2] = ignition? 0x03 : 0x01;

  crc = seq ^ 0xFF;
  for (i = 2; i <= 3; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = kStatusBuf[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_KlemmenStatus01[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  kStatusBuf[0] = crc;
  kStatusBuf[1] = seq;

  CAN.sendMsgBuf(KLEMMEN_STATUS_01_ID, 0, 4, kStatusBuf);
}

void VWMQBCluster::sendESP20() {
  //
  // Esp_20
  //

  crc = (0x30 | seq) ^ 0xFF;
  for (i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = esp20Buf[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_ESP20[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  esp20Buf[0] = crc;
  esp20Buf[1] = 0x30 | seq;

  CAN.sendMsgBuf(ESP_20_ID, 0, 8, esp20Buf);
}

void VWMQBCluster::sendESP21(int speed) {
  //
  // Esp_21
  //

  vSpeed = (unsigned long)speed * 98.5;
  esp21Buf[4] = vSpeed % 256;
  esp21Buf[5] = vSpeed / 256;

  crc = (0xD0 | seq) ^ 0xFF;
  for (i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = esp21Buf[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_ESP21[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  esp21Buf[0] = crc;
  esp21Buf[1] = 0xD0 | seq;

  CAN.sendMsgBuf(ESP_21_ID, 0, 8, esp21Buf);
  CAN.sendMsgBuf(MOTOR_14_ID, 0, 8, motor14Buf);

  if (!passthroughMode) {
    CAN.sendMsgBuf(GATEWAY_76_ID, 0, 8, gateway76Buf);
  }
}

void VWMQBCluster::sendBacklightBrightness(uint8_t brightness) {
  dimmungBuf[0] = map(brightness, 0, 100, 0, 255);
  CAN.sendMsgBuf(DIMMUNG_01_ID, 0, 8, dimmungBuf);
}

void VWMQBCluster::sendSteeringWheelControls(int button) {
  switch (button) {
    case 1: mfswBuf[0] = 0x01; mfswBuf[2] = 0x01; break; // Menu (does nothing)
    case 2: mfswBuf[0] = 0x02; mfswBuf[2] = 0x01; break; // Right (does nothing)
    case 3: mfswBuf[0] = 0x03; mfswBuf[2] = 0x01; break; // Left (does nothing)
    case 4: mfswBuf[0] = 0x06; mfswBuf[2] = 0x01; break; // Up
    case 5: mfswBuf[0] = 0x06; mfswBuf[2] = 0x0F; break; // Down
    case 6: mfswBuf[0] = 0x07; mfswBuf[2] = 0x01; break; // OK
    case 7: mfswBuf[0] = 0x21; mfswBuf[2] = 0x01; break; // Asterisk (does nothing)
    case 8: mfswBuf[0] = 0x23; mfswBuf[2] = 0x01; break; // View (does nothing)
  }
  CAN.sendMsgBuf(MFSW_ID, 0, 4, mfswBuf);
  delay(10);

  mfswBuf[0] = 0x00; mfswBuf[2] = 0x00;
  CAN.sendMsgBuf(MFSW_ID, 0, 4, mfswBuf);
}

void VWMQBCluster::sendTSK07() {
  //
  // Tsk 07
  //

  crc = (0xE0 | seq) ^ 0xFF;
  for (i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = tsk07Buf[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_TSK07[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  tsk07Buf[0] = crc;
  tsk07Buf[1] = 0xE0 | seq;
  CAN.sendMsgBuf(TSK_07_ID, 0, 8, tsk07Buf);
}

void VWMQBCluster::sendLhEPS01() {
  //
  // Lh_Eps_01
  //

  crc = (0x00 | seq) ^ 0xFF;
  for (i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = lhEps01Buf[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_LH_EPS01[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  lhEps01Buf[0] = crc;
  lhEps01Buf[1] = 0x00 | seq;

  CAN.sendMsgBuf(LH_EPS_01_ID, 0, 8, lhEps01Buf);
}

void VWMQBCluster::sendMotor(int rpm, int coolantTemperature, int oilTemperature) {
  //
  // Motor_Code_01
  //

  crc = (0x10 | seq) ^ 0xFF;
  for (i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = motorCode01Buf[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_MCODE01[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  motorCode01Buf[0] = crc;
  motorCode01Buf[1] = 0x10 | seq;

  CAN.sendMsgBuf(MOTOR_CODE_01_ID, 0, 8, motorCode01Buf);

  //
  // Motor
  //
  rpmVal = (unsigned long)rpm / 3;
  motor04Buf[3] = rpmVal % 256;
  motor04Buf[4] = rpmVal / 256;

  CAN.sendMsgBuf(MOTOR_04_ID, 0, 8, motor04Buf);

  motor07Buf[2] = 60 + constrain(oilTemperature, 49, 193);
  CAN.sendMsgBuf(MOTOR_07_ID, 0, 8, motor07Buf);

  uint8_t mappedVal = map(coolantTemperature, 50, 130, 0x80, 0xED);
  motor09Buf[0] = mappedVal;

  CAN.sendMsgBuf(MOTOR_09_ID, 0, 8, motor09Buf);
}

void VWMQBCluster::sendESP24() {
  // Convert vSpeed to mph for calculating distance
  // this doesn't get sent to the cluster
  double speed_mph = static_cast<double>(vSpeed) / 100.0 * 0.621371; 

  // Calculate distance increment for 50ms interval in miles
  // ESP24 is sent every 50ms
  double time_interval_hours = 0.05 / 3600.0; // Convert 50ms to hours
  double distance_increment_miles = speed_mph * time_interval_hours;

  // Convert distance increment to the cluster unit
  // (Approx) 7195 units represents 0.1 miles
  double distance_increment_units = distance_increment_miles * (7195.0 / 0.1);

  static double accumulated_distance_units = 0;
  accumulated_distance_units += distance_increment_units;

  // Prepare the distance value for CAN
  uint16_t distance_for_cluster = static_cast<uint16_t>(accumulated_distance_units); 
  esp24Buf[5] = distance_for_cluster & 0xFF; 
  esp24Buf[6] = (distance_for_cluster >> 8) & 0xFF; 

  // Stop the odometer from counting indefinitely
  if (vSpeed == 0) {
    accumulated_distance_units = 0;
  }

  // If you leave this uncapped, the TCS-off light will come on
  if (accumulated_distance_units >= 30000) {
    accumulated_distance_units = 0; 
  }

  // Calculate and update esp24Speed
  unsigned long esp24Speed = vSpeed * 1.35; 
  esp24Buf[2] = esp24Speed % 256; 
  esp24Buf[3] = esp24Speed / 256; 

  uint8_t crc = seq ^ 0xFF;
  for (int i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = esp24Buf[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_ESP24[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc) & 0xFF;

  esp24Buf[0] = crc;
  esp24Buf[1] = seq;

  CAN.sendMsgBuf(ESP_24_ID, 0, 8, esp24Buf);
}


void VWMQBCluster::sendGear(uint8_t gear) {
  //
  // Wba_03
  //

  uint8_t tempGear = 0;
  uint8_t tempGearSelector = 0;
  switch (gear) {
    case 0:
      tempGear = 0;
      tempGearSelector = 0;
      break;
    case 1 ... 9:
      tempGear = gear;
      tempGearSelector = 0x60;
      break;  // Use 0x50 for S instead of M
    case 10:
      tempGear = 0;
      tempGearSelector = 0x10;
      break;
    case 11:
      tempGear = 0;
      tempGearSelector = 0x20;
      break;
    case 12:
      tempGear = 0;
      tempGearSelector = 0x30;
      break;
    case 13:
      tempGear = 0;
      tempGearSelector = 0x40;
      break;
  }

  wba03Buf[1] = tempGearSelector | seq;
  wba03Buf[3] = tempGear;

  crc = ((wba03Buf[1] & 0xF0) | seq) ^ 0xFF;
  for (i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = wba03Buf[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_WBA03[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  wba03Buf[0] = crc;
  wba03Buf[1] = (wba03Buf[1] & 0xF0) | seq;

  CAN.sendMsgBuf(WBA_03_ID, 0, 8, wba03Buf);

  if (!passthroughMode) {
    CAN.sendMsgBuf(RKA_01_ID, 0, 8, rka01Buf); // TODO: Importnant for MFSW? Try it
  }
}

void VWMQBCluster::sendAirbag01() {
  //
  // Airbag_01
  //
  crc = (0x00 | seq) ^ 0xFF;
  for (i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = airbag01Buf[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_AIRBAG01[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  airbag01Buf[0] = crc;
  airbag01Buf[1] = 0x00 | seq;

  CAN.sendMsgBuf(AIRBAG_01_ID, 0, 8, airbag01Buf);
}

void VWMQBCluster::sendBlinkers(boolean leftTurningIndicator, boolean rightTurningIndicator, boolean turningIndicatorsBlinking) {
  //
  // Blinkers
  //

  // TODO: Figure out CRC if present/needed?
  uint8_t temp_turning_lights = 0 | (leftTurningIndicator ? 0xA : 0) | (rightTurningIndicator ? 0x14 : 0);
  if (turningIndicatorsBlinking == true) {
    turning_lights_counter = turning_lights_counter + 1;
    if (turning_lights_counter <= 8) {
    } else if (turning_lights_counter > 8 && turning_lights_counter < 16) {
      temp_turning_lights = 0;
    } else {
      turning_lights_counter = 0;
    }
  }

  blinkerBuff[1] = seq;
  blinkerBuff[3] = temp_turning_lights;
  CAN.sendMsgBuf(BLINKMODI_02_ID, 0, 8, blinkerBuff);
}

void VWMQBCluster::sendTPMS() {
  //
  // TPMS
  //

  CAN.sendMsgBuf(TPMS_ID, 0, 8, tpmsBuff);
}

void VWMQBCluster::sendSWA01() {
  //
  // SWA_01
  //
  // TODO: Figure out why this does not do anything - CRC was checked and is correct

  crc = (0x00 | seq) ^ 0xFF;
  for (i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = swa01Buff[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_SWA01[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  swa01Buff[0] = crc;
  swa01Buff[1] = seq;

  CAN.sendMsgBuf(SWA_01_ID, 0, 8, swa01Buff);
}

void VWMQBCluster::sendParkBrake(boolean handbrakeActive) {
  parkBrakeBuff[0] = handbrakeActive ? 0x04 : 0x00;
  // To use the auto hold (green) indicator
  //parkBrakeBuff[1] = handbrakeActive ? 0x01 : 0x00;
  CAN.sendMsgBuf(PARKBRAKE_ID, 0, 4, parkBrakeBuff);
}

void VWMQBCluster::sendLights(boolean highBeam, boolean rearFogLight) {
  lichtVorne01Buff[1] = highBeam ? 0x40 : 0x00;
  lichtVorne01Buff[2] = rearFogLight ? 0x03 : 0x04;
  //lichtVorne01Buff[2] = 0x08; // To enable auto high beam assist indicator

  CAN.sendMsgBuf(LICHT_VORNE_01_ID, 0, 8, lichtVorne01Buff);
}

void VWMQBCluster::sendOtherLights() {
  crc = (0xC0 | seq) ^ 0xFF;
  for (i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = lichtAnfBuff[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_LICHT_ANF[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  lichtAnfBuff[0] = crc;
  lichtAnfBuff[1] = 0xC0 | seq;
  CAN.sendMsgBuf(LICHT_ANF_ID, 0, 8, lichtAnfBuff);

  lichtHintenBuff[0] = seq;
  CAN.sendMsgBuf(LICHT_HINTEN_01_ID, 0, 8, lichtHintenBuff);
}

void VWMQBCluster::sendDoorStatus(boolean doorOpen) {
  doorStatusBuff[3] = doorOpen; // bit 0: left front, bit 1: right front, bit 2: left rear, bit 3: right rear, bit 4: trunk

  CAN.sendMsgBuf(DOOR_STATUS_ID, 0, 8, doorStatusBuff);
}

void VWMQBCluster::sendOutdoorTemperature(int temperature) {
  outdoorTempBuff[0] = (50 + temperature) << 1; // Bit shift 1 to the left since 0.5 is the first bit

  CAN.sendMsgBuf(OUTDOOR_TEMP_ID, 0, 8, outdoorTempBuff);
}

void VWMQBCluster::updateTestBuffer(uint8_t val0, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5, uint8_t val6, uint8_t val7) {
  testBuff[0] = val0;
  testBuff[1] = val1;
  testBuff[2] = val2;
  testBuff[3] = val3;
  testBuff[4] = val4;
  testBuff[5] = val5;
  testBuff[6] = val6;
  testBuff[7] = val7;
}

void VWMQBCluster::sendTestBuffers() {
  //
  // TESTING ONLY. TO BE REMOVED
  //
/*
  // Example 1: 17331110,44 50 10 07 01 // Date?
  // Example 2: 17331110,44 51 17 0C 1B // Time?

  testBuff[0] = 0xB0;
  testBuff[1] = 0x07;
  testBuff[2] = 0x34;
  testBuff[3] = 0x5E;
  testBuff[4] = 0x00;
  testBuff[5] = 0x01;
  testBuff[6] = 0x01;
  testBuff[7] = 0x00;
  CAN.sendMsgBuf(DATE_ID, 0, 8, testBuff);

  delay(10);

  testBuff[0] = 0x44;
  testBuff[1] = 0x51;
  testBuff[2] = 0x17;
  testBuff[3] = 0x0C;
  testBuff[4] = 0x1B;
  CAN.sendMsgBuf(DATE_ID, 0, 5, testBuff);

  delay(10);

  testBuff[0] = 0xF0;
  testBuff[1] = 0x00;
  testBuff[2] = 0x00;
  testBuff[3] = 0x00;
  testBuff[4] = 0x00;
  testBuff[5] = 0x00;
  testBuff[6] = 0x00;
  testBuff[7] = 0x00;
  CAN.sendMsgBuf(DATE_ID, 0, 8, testBuff);

  testBuff[0] = 0x44;
  testBuff[1] = 0x50;
  testBuff[2] = 0x17;
  testBuff[3] = 0x07;
  testBuff[4] = 0x01;
  CAN.sendMsgBuf(DATE_ID, 0, 5, testBuff);

  testBuff[0] = 0xC2;
  testBuff[1] = 0x00;
  testBuff[2] = 0x17;
  testBuff[3] = 0x07;
  testBuff[4] = 0x04;
  testBuff[5] = 0x13;
  testBuff[6] = 0x09;
  testBuff[7] = 0x00;
  CAN.sendMsgBuf(DATE_ID, 0, 8, testBuff);*/
}

void VWMQBCluster::handleReceivedData(long unsigned int canRxId, unsigned char canRxLen, unsigned char canRxBuf[]) {
  if (passthroughMode == false) {
    return;
  }

  bool unknownId = false;

  switch (canRxId) {
    case ESP_20_ID: /*mqbDash.sendESP20();*/ break;
    case ESP_21_ID: /*mqbDash.sendESP21(speed * speedCorrectionFactor);*/ break;
    case TSK_07_ID: /*mqbDash.sendTSK07();*/ break;
    case LH_EPS_01_ID: /*mqbDash.sendLhEPS01();*/ break;
    case MOTOR_CODE_01_ID: /*mqbDash.sendMotor01();*/ break;
    case MOTOR_04_ID: /*mqbDash.sendMotor04(rpm*rpmCorrectionFactor);*/ break;
    case MOTOR_09_ID: /*mqbDash.sendMotor09(coolantTemperature);*/ break;
    case ESP_24_ID: /*mqbDash.sendESP24();*/ break;
    case WBA_03_ID: /*mqbDash.sendGear(gear);*/ break;
    case AIRBAG_01_ID: /*mqbDash.sendAirbag01();*/ break;
    case TPMS_ID: /*mqbDash.sendTPMS();*/ break;
    case MOTOR_14_ID: break;
    //case 0x104: break;
    case PARKBRAKE_ID: break;
    //case ESP_05_ID: break; // block only
    case 0x65a: break; // BCM01 block // gets rid of brake fluid warnings
    case ESP_10_ID: break; // block
    case ESP_02_ID: break; // block
    case MOTOR_18_ID: break;
    case MOTOR_26_ID: break;
    case MOTOR_07_ID: break;
    case OUTDOOR_TEMP_ID: break;
    case DOOR_STATUS_ID: break;
    case LICHT_HINTEN_01_ID: break;
    case LICHT_ANF_ID: break;
    case LICHT_VORNE_01_ID: break; // Intercept and decode this message to get the steering wheel stalk position
    default: unknownId = true; CAN.sendMsgBuf(canRxId, canRxLen, canRxBuf); break;
  }
}