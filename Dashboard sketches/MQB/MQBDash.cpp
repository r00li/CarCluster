// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "MQBDash.h"
#include "MQBCRC.h"

#include <mcp_can.h>

MQBDash::MQBDash(MCP_CAN& CAN): CAN(CAN) {
  //CAN = CANObject;
}

void MQBDash::updateWithState(int speed,
                              int rpm,
                              uint8_t backlightBrightness,
                              boolean leftTurningIndicator,
                              boolean rightTurningIndicator,
                              boolean turningIndicatorsBlinking,
                              uint8_t gear,
                              int coolantTemperature,
                              boolean handbrake,
                              boolean highBeam,
                              boolean rearFogLight,
                              boolean doorOpen,
                              int outdoorTemperature,
                              boolean ignition) {
  if (millis() - lastDashboardUpdateTime >= dashboardUpdateTime50) {
    // This should probably be done using a more sophisticated method like a
    // scheduler, but for now this seems to work.

    sendIgnitionStatus(ignition);
    sendBacklightBrightness(backlightBrightness);
    sendESP20();
    sendESP21(speed);
    sendTSK07();
    sendLhEPS01();
    sendMotor(rpm, coolantTemperature);
    sendESP24();
    sendGear(gear);
    sendAirbag01();
    sendBlinkers(leftTurningIndicator, rightTurningIndicator, turningIndicatorsBlinking);
    sendParkBrake(handbrake);
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
    sendLights(highBeam, rearFogLight);
    sendDoorStatus(doorOpen);
    sendOutdoorTemperature(outdoorTemperature);
    
    lastDashboardUpdateTime500ms = millis();
  }
}

void MQBDash::sendIgnitionStatus(boolean ignition) {
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

void MQBDash::sendESP20() {
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

void MQBDash::sendESP21(int speed) {
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
  CAN.sendMsgBuf(GATEWAY_76_ID, 0, 8, gateway76Buf);
}

void MQBDash::sendBacklightBrightness(uint8_t brightness) {
  dimmungBuf[0] = map(brightness, 0, 100, 0, 255);
  CAN.sendMsgBuf(DIMMUNG_01_ID, 0, 8, dimmungBuf);
}

void MQBDash::sendSteeringWheelControls(int button) {
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

void MQBDash::sendTSK07() {
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

void MQBDash::sendLhEPS01() {
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

void MQBDash::sendMotor(int rpm, int coolantTemperature) {
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

  // CAN.sendMsgBuf(MOTOR_07_ID, 0, 8, motor07Buf); // TODO: REMOVE

  uint8_t mappedVal = map(coolantTemperature, 50, 130, 0x80, 0xED);
  motor09Buf[0] = mappedVal;

  CAN.sendMsgBuf(MOTOR_09_ID, 0, 8, motor09Buf);
}

void MQBDash::sendESP24() {
  //
  // Esp_24
  //

  esp24Speed = (unsigned long)vSpeed * 1.35;
  esp24Inc = (unsigned long)esp24Speed / 240;
  esp24Buf[2] = esp24Speed % 256;
  esp24Buf[3] = esp24Speed / 256;
  esp24Distance = esp24Distance + esp24Inc;
  if (esp24Speed == 0) { 
    esp24Distance = 0; 
  }
  if (esp24Distance > 0x7ff) {
    esp24Overflow = 0x10;
    esp24Distance = esp24Distance % 0x800;
  }

  esp24Buf[5] = esp24Distance % 256;
  esp24Buf[6] = esp24Overflow | (esp24Distance / 256);

  crc = seq ^ 0xFF;
  for (i = 2; i <= 7; i++) {
    crc = P_L_CC_CRC_LUT_APV[crc];
    crc = esp24Buf[i] ^ crc;
  }
  crc = P_L_CC_CRC_LUT_APV[crc] ^ P_L_CC_KENNUNG_APV_ESP24[seq];
  crc = P_L_CC_CRC_LUT_APV[crc];
  crc = (~crc);

  esp24Buf[0] = crc;
  esp24Buf[1] = seq;

  // TODO: Check if can be removed
  // while(millis()<prevTime+45);
  // prevTime=millis();
  CAN.sendMsgBuf(ESP_24_ID, 0, 8, esp24Buf);
}

void MQBDash::sendGear(uint8_t gear) {
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

  CAN.sendMsgBuf(RKA_01_ID, 0, 8, rka01Buf); // TODO: Importnant for MFSW? Try it
}

void MQBDash::sendAirbag01() {
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

void MQBDash::sendBlinkers(boolean leftTurningIndicator, boolean rightTurningIndicator, boolean turningIndicatorsBlinking) {
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

void MQBDash::sendTPMS() {
  //
  // TPMS
  //

  CAN.sendMsgBuf(TPMS_ID, 0, 8, tpmsBuff);
}

void MQBDash::sendSWA01() {
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

void MQBDash::sendParkBrake(boolean handbrakeActive) {
  parkBrakeBuff[0] = handbrakeActive ? 0x04 : 0x00;
  // To use the auto hold (green) indicator
  //parkBrakeBuff[1] = handbrakeActive ? 0x01 : 0x00;
  CAN.sendMsgBuf(PARKBRAKE_ID, 0, 4, parkBrakeBuff);
}

void MQBDash::sendLights(boolean highBeam, boolean rearFogLight) {
  lichtVorne01Buff[1] = highBeam ? 0x40 : 0x00;
  lichtVorne01Buff[2] = rearFogLight ? 0x03 : 0x04;
  //lichtVorne01Buff[2] = 0x08; // To enable auto high beam assist indicator

  CAN.sendMsgBuf(LICHT_VORNE_01_ID, 0, 8, lichtVorne01Buff);
}

void MQBDash::sendDoorStatus(boolean doorOpen) {
  doorStatusBuff[3] = doorOpen; // bit 0: left front, bit 1: right front, bit 2: left rear, bit 3: right rear, bit 4: trunk

  CAN.sendMsgBuf(DOOR_STATUS_ID, 0, 8, doorStatusBuff);
}

void MQBDash::sendOutdoorTemperature(int temperature) {
  outdoorTempBuff[0] = (50 + temperature) << 1; // Bit shift 1 to the left since 0.5 is the first bit

  CAN.sendMsgBuf(OUTDOOR_TEMP_ID, 0, 8, outdoorTempBuff);
}

void MQBDash::updateTestBuffer(uint8_t val0, uint8_t val1, uint8_t val2, uint8_t val3, uint8_t val4, uint8_t val5, uint8_t val6, uint8_t val7) {
  testBuff[0] = val0;
  testBuff[1] = val1;
  testBuff[2] = val2;
  testBuff[3] = val3;
  testBuff[4] = val4;
  testBuff[5] = val5;
  testBuff[6] = val6;
  testBuff[7] = val7;
}

void MQBDash::sendTestBuffers() {
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