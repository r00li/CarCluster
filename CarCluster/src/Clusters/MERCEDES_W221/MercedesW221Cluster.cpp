// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "MercedesW221Cluster.h"

MercedesW221Cluster::MercedesW221Cluster(MCP_CAN& CAN): CAN(CAN) {
}

uint8_t MercedesW221Cluster::mapGenericGearToLocalGear(GearState inputGear) {
  // 0x01-7- 1-7, 0x50=P, 0x52=R, 0x4e=N, 0x44=D, 0x31-37 =D1-D7

  switch(inputGear) {
    case GearState_Manual_1: return 0x01; break;
    case GearState_Manual_2: return 0x02; break;
    case GearState_Manual_3: return 0x03; break;
    case GearState_Manual_4: return 0x04; break;
    case GearState_Manual_5: return 0x05; break;
    case GearState_Manual_6: return 0x06; break;
    case GearState_Manual_7: return 0x07; break;
    case GearState_Manual_8: return 0x44; break;
    case GearState_Manual_9: return 0x44; break;
    case GearState_Manual_10: return 0x44; break;
    case GearState_Auto_P: return 0x50; break;
    case GearState_Auto_R: return 0x52; break;
    case GearState_Auto_N: return 0x4e; break;
    case GearState_Auto_D: return 0x44; break;
    case GearState_Auto_S: return 0x44; break;
  }
}

int MercedesW221Cluster::mapSpeed(GameState& game) {
  int scaledSpeed = game.speed * game.configuration.speedCorrectionFactor;
  if (scaledSpeed > game.configuration.maximumSpeedValue) {
    return game.configuration.maximumSpeedValue;
  } else {
    return scaledSpeed;
  }
}

int MercedesW221Cluster::mapRPM(GameState& game) {
  int scaledRPM = game.rpm * game.configuration.speedCorrectionFactor;
  if (scaledRPM > game.configuration.maximumRPMValue) {
    return game.configuration.maximumRPMValue;
  } else {
    return scaledRPM;
  }
}

int MercedesW221Cluster::mapCoolantTemperature(GameState& game) {
  if (game.coolantTemperature < game.configuration.minimumCoolantTemperature) { return game.configuration.minimumCoolantTemperature; }
  if (game.coolantTemperature > game.configuration.maximumCoolantTemperature) { return game.configuration.maximumCoolantTemperature; }
  return game.coolantTemperature;
}

void MercedesW221Cluster::updateWithGame(GameState& game) {
  if (millis() - lastDashboardUpdateTime >= dashboardUpdateTime100) {
    // This should probably be done using a more sophisticated method like a
    // scheduler, but for now this seems to work.
 
    sendIgnition(game.ignition);
    sendBasicDriveParameters(mapSpeed(game), mapRPM(game));
    sendGear(mapGenericGearToLocalGear(game.gear));
    sendCoolantTemperature(mapCoolantTemperature(game));
    sendBlinkers(game.leftTurningIndicator, game.rightTurningIndicator);
    sendFuel(game.fuelQuantity);
    sendAbs(game.absLight, game.offroadLight);
    sendStatusMessages(game.doorOpen, game.highBeam, game.outdoorTemperature);
    sendParkingBrake(game.handbrake);
    sendOthers();

    if (game.buttonEventToProcess != 0) {
      sendSteeringWheelControls(game.buttonEventToProcess);
      game.buttonEventToProcess = 0;
    } else {
      sendSteeringWheelControls(0);
    }

    count++;
    if (count >= 254) { count = 0; } // Needs to be reset at 254 not 255

    lastDashboardUpdateTime = millis();
  }
}

void MercedesW221Cluster::sendIgnition(bool ignition) {
  // Ignition
  uint8_t IGN = ignition ? 0xCC : 0xCF;
  // Byte 0: 0xc2 = acc, 0xcc = ignition, 0x07 = crank, 0xCF = off
  unsigned char ignitionBuff[] = { IGN, 0x80, 0xCF, 0xAD, 0xAA, 0x07, 0x10, 0x55 };
  CAN.sendMsgBuf(0x001, 0, 8, ignitionBuff);

  // Key status
  // Byte 1: 1 key being int, 2 =replace key,
  // 8 =shift into P or N to start vehicle,
  // 32 =key doesnt match, 0x64 no key,
  // 128 =remove stop button and insert key
  unsigned char keyBuff[] = { 0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  CAN.sendMsgBuf(0x2f8, 0, 8, keyBuff);
}

void MercedesW221Cluster::sendBasicDriveParameters(int speed, int rpm) {
  // RPM
  int tempRPM = rpm * 3.84;
  uint8_t rpmL = lo8(tempRPM);
  uint8_t rpmH = hi8(tempRPM);
  unsigned char rpmBuff[] = { rpmH*0.27, rpmH*0.27, 0x7F, 0x00, 0x00, 0xC9, 0x63, 0x1A };
  CAN.sendMsgBuf(0x105, 0, 8, rpmBuff);

  // Speedometer
  int speedTmp = speed/0.0072; //KMH=1.12 MPH=0.62
  uint8_t speedL = lo8(speedTmp);
  uint8_t speedH = hi8(speedTmp);
  unsigned char speedBuff[] = { speedH*0.125, speedH*0.125, speedH*0.125, speedH*0.125, speedH*0.125, speedH*0.125, speedH*0.125, speedH*0.125 };
  CAN.sendMsgBuf(0x203, 0, 8, speedBuff);
}

void MercedesW221Cluster::sendGear(int gear) {
  // Trans1
  unsigned char transmission1Buffer[] = { gear, 0, 0x00, 0x00, 0x00, 0x01, 0x00, count };
  CAN.sendMsgBuf(0x0F3, 0, 8, transmission1Buffer);

  // Gearbox keep alive
  // Byte 3: errors
  CAN.sendMsgBuf(0x2F5, 0, 8, transmission1Buffer);
  
  // Drivemode and gears
  // Byte 1: 0x01-7- 1-7, 0x50=P, 0x52=R, 0x4e=N, 0x44=D, 0x31-37 =D1-D7, 
  // Byte 2: Drivemodes Auto=65, Comfort=67, Eco=69, Eco+ =71, M=77, Sport=83, Sport+ =84,
  // Byte 3,5: errors,kills gears
  // Byte 7: 5,20 Transmission Malfunction, Revers not poss, 1,30=In risk of vehicle Rolling trans not in P, =race start active
  unsigned char driveModeBuffer[] = { gear, 84, 0x00, 0x00, 0x00, 0x01, 0, count };
  CAN.sendMsgBuf(0x2F3, 0, 8, driveModeBuffer);
}

void MercedesW221Cluster::sendCoolantTemperature(int temperature) {
  // Byte 0: water temp
  // Byte 2: oil temp
  uint8_t coolantTempScaled = map(temperature, 40, 120, 77, 161);
  unsigned char coolantTempBuffer[] = { coolantTempScaled, 0xf0, 0x96, 0x80, 0x48, 0x1A, 0x8F, 0x09 }; // TODO: Check if 0 should be sent instead
  CAN.sendMsgBuf(0x30d, 0, 8, coolantTempBuffer);
}

void MercedesW221Cluster::sendBlinkers(bool leftBlinker, bool rightBlinker) {
  // Turn indicator
  // Byte2: 0x01 = left indicator, 0x02 = right indicator
  uint8_t indicators = leftBlinker && rightBlinker ? 250 : (leftBlinker ? 100 : (rightBlinker ? 150 : 0)); // 150 - RIGHT | 100 - LEFT | 250 - HAZARD
  unsigned char blinkerBuffer[] = { indicators, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  CAN.sendMsgBuf(0x029, 0, 8, blinkerBuffer);
}

void MercedesW221Cluster::sendAbs(bool absOn, bool tractionOn) {
  // ABS and brakes
  // Byte 1: Brakes overheated
  // Byte 2: ABS tc ebd unavailable
  // Byte 3: ABS, Traction
  // Byte 5: HOLD=0x2f, ECO?(C Class not found)
  // Byte 7: DSR Waring messages and power steering
  // Byte 8: Dsr ON/OFF
  unsigned char absBuffer[] = { 0x00, 0x00, (tractionOn | absOn << 2), 0x00, 0x00, 0, 0x00, 0x00 };
  CAN.sendMsgBuf(0x247, 0, 8, absBuffer);
}

void MercedesW221Cluster::sendFuel(int fuelPercentage) {
  int fuela = 0;
  int fuelb = 0;
  if(fuelPercentage > 40) {
    fuela = 0;
    fuelb = 210-(fuelPercentage-40)*3.166; 
  } else {
    fuelb = 210;
    fuela = (fuelPercentage-40)*-5.25;
  }
  unsigned char fuelBuffer[] = { fuela, fuelb, 0, 0, 0, 0, 0, 0 };
  CAN.sendMsgBuf(0xF8, 0, 8, fuelBuffer);
}

void MercedesW221Cluster::sendStatusMessages(bool open, bool highBeam, int outdoorTemp) {
  uint8_t temp = (outdoorTemp + 40) * 2;
  unsigned char doorBuffer[] = { open ? 2 : 0, 0, 0, highBeam, 0, temp, 0, 0 };
  CAN.sendMsgBuf(0x139, 0, 8, doorBuffer);
}

void MercedesW221Cluster::sendParkingBrake(bool parkingBrake) {
  unsigned char parkBrakeBuff[] = {0, (parkingBrake ? 0x04 : 0x00), 0, 0, 0, 0, 0, 0 };
  CAN.sendMsgBuf(0x17A, 0, 8, parkBrakeBuff);
}

void MercedesW221Cluster::sendOthers() {
  // Traction
  unsigned char tractionBuffer[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  CAN.sendMsgBuf(0x005, 0, 8, tractionBuffer);

  // Airbag
  unsigned char airbagBuffer[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  CAN.sendMsgBuf(0x375, 0, 8, airbagBuffer);

  // Distronic
  unsigned char distronicBuffer[] = { 0, 0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  CAN.sendMsgBuf(0x378, 0, 8, distronicBuffer);

  // Glow plugs and EML    
  // Byte 1: EML, Clean fuel filter, temp goes lower?
  // Byte 2: Auto Stop Start ON=0x08 , OFF= 0x00 CLA |             1---9v                          1---9^
  // Byte 4; 0x01-0a is shift  (numbs only arrows) c class only | 0x11-1a recommended down shift, 0x21-2a is Upshift
  // Byte 6: Engine Oil Pressure warning 0x30
  // Byte 7: Clutch Overheated Engage/Disengage QUICKLY
  // Byte 8: Depress clutch to fully start engine
  unsigned char glowPlugBuffer[] = { 0x00, 0x00, 0x00, 0, 0x00, 0x00, 0x00, 0x10 };
  CAN.sendMsgBuf(0x33d, 0, 8, glowPlugBuffer);
 
  // Power steering
  unsigned char powerSteeringBuffer[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  CAN.sendMsgBuf(0x340, 0, 8, powerSteeringBuffer);

  // Tpms
  unsigned char tpmsBuffer[] = { 0x00, 0x00, 0x00, 0x20, 0x40, 0x40, 0x40, 0x40 };
  CAN.sendMsgBuf(0x2ff, 0, 8, tpmsBuffer);
   
  // Park assist, TPMS, ...
  // Byte 1: taxi errors 
  // Byte 2: radio 30=switch radio on with +,
  // Byte 4: Parking assist
  // Byte 5: Parking guidence, blind sport monitor 
  // Byte 6: is Tpms Inopertive 
  unsigned char errorBuffer[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  CAN.sendMsgBuf(0x206, 0, 8, errorBuffer);

  // SOS, Light faults, Pre-safe
  // Byte 1: PRE SAFE inopertive 
  // Byte 3: is Blindness,speed assist inop
  CAN.sendMsgBuf(0x208, 0, 8, errorBuffer);

  // Brake fluid low error
  CAN.sendMsgBuf(0x30e, 0, 8, errorBuffer);

  // Airtronic
  CAN.sendMsgBuf(0x379, 0, 8, errorBuffer);

  // Brake fluid level
  CAN.sendMsgBuf(0x2FA, 0, 8, errorBuffer);

  // avoid 204 hex - VIN number

  // To switch language to english:
  // CAN ID: 861, DATA: 10, 10, 10, 10, 10, 10, 10, 10
}

void MercedesW221Cluster::sendSteeringWheelControls(int button) {
  unsigned char buttonCommand[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  switch(button) {
    case 1: buttonCommand[4] = 0x01; break; // Up
    case 2: buttonCommand[4] = 0x02; break; // Down
    case 3: buttonCommand[4] = 0x08; break; // Left
    case 4: buttonCommand[4] = 0x04; break; // Right
    case 5: buttonCommand[5] = 0x48; break; // Ok
  }
  CAN.sendMsgBuf(0x45, 0, 8, buttonCommand);
}
