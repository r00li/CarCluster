// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "PQ46Dash.h"

#include <mcp_can.h>

// Helper constants
#define lo8(x) ((int)(x)&0xff)
#define hi8(x) ((int)(x) >> 8)

PQ46Dash::PQ46Dash(MCP_CAN& CAN): CAN(CAN) {
  //CAN = CANObject;
}

void PQ46Dash::updateWithState(int speed,
                               int rpm,
                               uint8_t backlightBrightness,
                               boolean leftBlinker,
                               boolean rightBlinker,
                               boolean blinkersBlinking,
                               boolean daylightBeam,
                               boolean highBeam,
                               boolean frontFogLight,
                               boolean rearFogLight,
                               boolean batteryWarning,
                               boolean trunkOpen,
                               boolean doorOpen,
                               boolean tpmsLight,
                               boolean espLight,
                               boolean absLight,
                               uint8_t gear,
                               int coolantTemperature) {
  if (millis() - lastDashboardUpdateTime >= dashboardUpdateTime50) {
    // This should probably be done using a more sophisticated method like a
    // scheduler, but for now this seems to work.

    sendImmobilizer();
    sendIndicators(leftBlinker, rightBlinker, blinkersBlinking, daylightBeam, highBeam, frontFogLight, rearFogLight, batteryWarning, trunkOpen, doorOpen, backlightBrightness);
    sendDieselEngine();
    sendRPM(rpm);
    sendSpeed(speed, tpmsLight, espLight, absLight);
    sendABS(speed);
    sendGear(gear);
    sendAirbag(false);

    seq++;
    if (seq > 15) {
      seq = 0;
    }

    lastDashboardUpdateTime = millis();
  }

  if (millis() - lastDashboardUpdateTime500ms >= dashboardUpdateTime500) {
    
    lastDashboardUpdateTime500ms = millis();
  }
}

void PQ46Dash::sendImmobilizer() {
  CAN.sendMsgBuf(IMMOBILIZER_ID, 0, 8, immobilizerBuffer);
}

void PQ46Dash::sendIndicators(boolean leftBlinker, boolean rightBlinker, boolean blinkersBlinking, boolean daylightBeam, boolean highBeam, boolean frontFogLight, boolean rearFogLight, boolean batteryWarning, boolean trunkOpen, boolean doorOpen, int brightness) {
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

  //CanSend(0x470, temp_battery_warning + temp_turning_lights, temp_trunklid_open + door_open, (backlight ? backlight_brightness | 1 : 0), 0x00, temp_check_lamp + temp_clutch_control, temp_keybattery_warning, 0x00, lightmode);
  indicatorsBuffer[0] = temp_battery_warning;
  indicatorsBuffer[1] = temp_trunklid_open + doorOpen;
  indicatorsBuffer[2] = 99; // TODO: Figure out where backlight brightness is on PQ46. 
  indicatorsBuffer[4] = temp_check_lamp + temp_clutch_control;
  indicatorsBuffer[5] = temp_keybattery_warning;
  CAN.sendMsgBuf(INDICATORS_ID, 0, 8, indicatorsBuffer);

  lightsBuffer[0] = (daylightBeam << 1) | (highBeam << 2) | (frontFogLight << 3) | (rearFogLight << 4);
  lightsBuffer[2] = temp_turning_lights;
  //CanSend(0x531, speed, 0x00, temp_turning_lights, 0x00, 0x00, 0x00, 0x00, 0x00);
  CAN.sendMsgBuf(LIGHTS_ID, 0, 8, lightsBuffer);
}

void PQ46Dash::sendDieselEngine() {
  CAN.sendMsgBuf(DIESEL_ENGINE_ID, 0, 8, diselEngineBuffer);
}

void PQ46Dash::sendRPM(int rpm) {
  rpmBuffer[2] = (uint8_t)((rpm * 4) & 0x00FF);
  rpmBuffer[3] = (uint8_t)((rpm * 4) >> 8);

  CAN.sendMsgBuf(RPM_ID, 0, 8, rpmBuffer);
}

void PQ46Dash::sendSpeed(int speed, boolean tpmsLight, boolean espLight, boolean absLight) {
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

void PQ46Dash::sendABS(int speed) {
  absBuffer[2] = lo8(speed);
  absBuffer[3] = hi8(speed);
  absBuffer[7] = (0x10 | seq);

  CAN.sendMsgBuf(ABS_ID, 0, 8, absBuffer);
}

void PQ46Dash::sendGear(uint8_t gear) {
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

void PQ46Dash::sendAirbag(boolean seatBeltLight) {
  airbagBuffer[2] = (seatBeltLight << 2);

  CAN.sendMsgBuf(AIRBAG_ID, 0, 8, airbagBuffer);
}