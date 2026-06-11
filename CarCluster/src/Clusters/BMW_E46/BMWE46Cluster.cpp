// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// E46 Cluster implemented by @NCPlyn
// 
// ####################################################################################################################

#include "BMWE46Cluster.h"

BMWE46Cluster::BMWE46Cluster(MCP_CAN& CAN, int fuelPotIncPin, int fuelPotDirPin, int fuelPot1CsPin, int fuelPot2CsPin, int FhandbrakePin, int FspeedPin, int FabsPin, bool fConsumption): CAN(CAN) {
  fuelPot.begin(fuelPotIncPin, fuelPotDirPin, fuelPot1CsPin);
  fuelPot.setPosition(100, true); // Force the pot to a known value

  fuelPot2.begin(fuelPotIncPin, fuelPotDirPin, fuelPot2CsPin);
  fuelPot2.setPosition(100, true); // Force the pot to a known value

  this->handbrakePin = FhandbrakePin;
  this->speedPin = FspeedPin;
  this->absPin = FabsPin;
  this->fakeConsumption = fConsumption;

  pinMode(FhandbrakePin, INPUT);
  pinMode(FabsPin, OUTPUT);
  digitalWrite(FabsPin, LOW);
  pinMode(FspeedPin, OUTPUT);
  noTone(FspeedPin);

  Serial1.begin(9600, SERIAL_8E1, 16, 17); //KBUS
  
  // Turns on backlight for 2004+ IKE
  //https://diolum.fr/analyse-lsz-e46 (I use from different forum post, is brighter)
  byte kbusBacklight[] = {0xD0, 0x07, 0xBF, 0x5C, 0xA8, 0x2A, 0xFF, 0x00, 0x49};
  Serial1.write(kbusBacklight, sizeof(kbusBacklight));
  
  // Sets some time for cluster instead of blank
  //https://web.archive.org/web/20180208213215/http://web.comhem.se/bengt-olof.swing/IBus.htm
  byte kbusTime[] = {0x3B, 0x06, 0x80, 0x40, 0x01, 0x0C, 0x3B, 0xCB}; //sets time
  Serial1.write(kbusTime, sizeof(kbusTime));
}

int BMWE46Cluster::mapSpeed(GameState& game) {
  int scaledSpeed = game.speed * game.configuration.speedCorrectionFactor;
  if (scaledSpeed > game.configuration.maximumSpeedValue) {
    return game.configuration.maximumSpeedValue;
  } else {
    return scaledSpeed;
  }
}

int BMWE46Cluster::mapRPM(GameState& game) {
  int scaledRPM = game.rpm * game.configuration.speedCorrectionFactor;
  if (scaledRPM > game.configuration.maximumRPMValue) {
    return game.configuration.maximumRPMValue;
  } else {
    return scaledRPM;
  }
}

int BMWE46Cluster::mapCoolantTemperature(GameState& game) {
  if (game.coolantTemperature < game.configuration.minimumCoolantTemperature) { return game.configuration.minimumCoolantTemperature; }
  if (game.coolantTemperature > game.configuration.maximumCoolantTemperature) { return game.configuration.maximumCoolantTemperature; }
  return game.coolantTemperature;
}

void BMWE46Cluster::updateWithGame(GameState& game) {
  if (millis() - last10 >= 10) { //every 10ms
    sendDME1(mapRPM(game));
    sendDME2(mapCoolantTemperature(game));
    sendDME4(mapRPM(game), (mapCoolantTemperature(game)>125));
    sendSpeed(mapSpeed(game));
    sendASC1(game.offroadLight);
    sendEGS1(game.gear);
    last10 = millis();
  }
  
  if(millis() - last20 >= 20) { //every 20ms
    sendASC3();
    last20 = millis();
  }

  if (millis() - last500 >= 500) { //every 500ms
    sendKBus(game.mainLights, game.highBeam, game.frontFogLight, game.rearFogLight, game.leftTurningIndicator, game.rightTurningIndicator, game.doorOpen);
    sendIO(game.handbrake,game.absLight);
    setFuel(game);
    last500 = millis();
  }
}

int BMWE46Cluster::gearStateToDisplayCode(GearState state) {
  switch (state) {
    case GearState_Manual_1: return 1;
    case GearState_Manual_2: return 2;
    case GearState_Manual_3: return 3;
    case GearState_Manual_4: return 4;
    case GearState_Manual_5: return 9;
    case GearState_Manual_6: return 10;
    case GearState_Auto_D:   return 5;
    case GearState_Auto_N:   return 6;
    case GearState_Auto_R:   return 7;
    case GearState_Auto_S:   return 0;
    case GearState_Auto_P:   return 8;
    default:                 return 0; // Fallback to display off
  }
}

// Automatic gearbox: show gears on IKE
//https://nam3forum.com/forums/forum/special-interests/coding-tuning/46634-smg-can-bus-decoding/page4
//https://www.bimmerforums.com/forum/showthread.php?1887229-E46-Can-bus-project&p=30055342#post30055342
void BMWE46Cluster::sendEGS1(GearState gear) {
  uint8_t EGS1[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

  EGS1[1] = gearStateToDisplayCode(gear); // Byte 1: Right Gear display 
  
  uint8_t tmp = ~(EGScounter ^ EGS1[1]) & 0x0F; // Byte 3: Checksum and counter
  EGS1[3] = (tmp << 4) | (EGScounter & 0x0F);
  
  if(gear == GearState_Auto_S) {
    EGS1[2] = 0x40; // Byte 2: Left Gear display: 0x20 = M; 0x40 = S; 0x00 = E; 0xFF = _
  } else {
    EGS1[2] = 0x20;
  }
  
  CAN.sendMsgBuf(0x43F, 0, 8, EGS1);
  EGScounter = (EGScounter + 1) & 0x0F; //Increment and wrap the counter (0-15)
}

// Shows ASC light (intervention)
void BMWE46Cluster::sendASC1(bool tractionLight) {
  uint8_t ASC1[8] = {0x00,(tractionLight)?0x01:0x00,0x27,0xFF,0x00,0xFF,0xFF,0x80};
  CAN.sendMsgBuf(0x153, 0, 8, ASC1);
}

// Shuts off ABS and ASC light on newgen IKE
//https://www.bimmerforums.com/forum/showthread.php?1887229-E46-Can-bus-project&p=29306929#post29306929
void BMWE46Cluster::sendASC3() {
  uint8_t ASC3[8] = {0x40,0x80,0x00,0xFF,0x41,0x7F,0x00,0x08};
  CAN.sendMsgBuf(0x1F3, 0, 8, ASC3);
}

// Moves RPM gauge
//https://www.ms4x.net/index.php?title=Siemens_MS43_CAN_Bus#DME1_0x316
//DME1; ID=0x316; data[2]=LSB, data[3]=MSB : (rpm*6.4 = b3&&b2)
void BMWE46Cluster::sendDME1(int rpm) {
  int clusterRPM = rpm*6.4;
  uint8_t DME1[8] = {0x05,0x14,clusterRPM & 0xff,clusterRPM >> 8,0x14,0x17,0x00,0x16};
  CAN.sendMsgBuf(0x316, 0, 8, DME1);
}

// Generates pulses from hall effect ABS sensor for speed gauge
void BMWE46Cluster::sendSpeed(int speed) {
  if(speed < 1) {
    noTone(this->speedPin);
  } else if (this->lastSpeed != speed) {
    tone(this->speedPin, 6.6 * speed);
    this->lastSpeed = speed;
  }
}

// Moves temperature gauge
//https://www.ms4x.net/index.php?title=Siemens_MS43_CAN_Bus#DME2_0x329
//DME2; ID=0x329; temp = data[1] : (bin = (c+48,373)/0,75)
void BMWE46Cluster::sendDME2(int coolantTemperature) { //done
  uint8_t DME2[] = {0x07,(uint8_t)((coolantTemperature+48.373)/0.75),0xB2,0x19,0x00,0xEE,0x00,0x00};
  CAN.sendMsgBuf(0x329, 0, 8, DME2);
}

// Moves consumption gauge & high temperature light
//Can also do CEL, Fuel cap light, M-warmup LEDs, oil temp/level, battery charge....
//https://www.ms4x.net/index.php?title=Siemens_MS43_CAN_Bus#DME4_0x545
//DME4; ID=0x0x545; data[1]=LSB, data[2]=MSB (rate of change)
void BMWE46Cluster::sendDME4(int rpm, bool tempWarn) {
  uint8_t DME4[] = {0x00,0x00,0x00,0x00,0x7E,0x10,0x00,0x18};

  if(this->fakeConsumption) {
    if(this->mpgloop == 0xFFFF) {
      this->mpgloop = 0x0;
    } else {
      this->mpgloop += rpm/150;
    }
    DME4[1] = this->mpgloop & 0xff;
    DME4[2] = this->mpgloop >> 8;
  }

  bitWrite(DME4[3],3,tempWarn);

  CAN.sendMsgBuf(0x545, 0, 8, DME4);
}

// Sets digital IO of handbrake and ABS light
void BMWE46Cluster::sendIO(bool handbrake, bool absLight) {
  if (handbrake) {
    pinMode(this->handbrakePin, OUTPUT);
    digitalWrite(this->handbrakePin, LOW);
  } else {
    pinMode(this->handbrakePin, INPUT);
  }
  if (absLight) {
    digitalWrite(this->absPin, HIGH);
  } else {
    digitalWrite(this->absPin, LOW);
  }
}

// Sends KBUS messages
//Atm blinkers, fogs, highbeam, car icon
//Can do doors, faulty lights... no idea how
void BMWE46Cluster::sendKBus(bool mainLights, bool highBeam, bool frontFogLight, bool rearFogLight, bool leftTurningIndicator, bool rightTurningIndicator, bool doors) {
  byte kbusmsg[10] = {0xD0, 0x08, 0xBF, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcc}; //last 0xcc is chksum

  if (rightTurningIndicator) { //right
    bitWrite(kbusmsg[4],6,1);
    lastIndicatorR = millis();
  } else {
    if(lastIndicatorR+500 < millis()) {
      bitWrite(kbusmsg[4],6,0);
    }
  }
  
  if (leftTurningIndicator) { //left
    bitWrite(kbusmsg[4],5,1); //leftBlink
    lastIndicatorL = millis();
  } else {
    if(lastIndicatorL+500 < millis()) {
      bitWrite(kbusmsg[4],5,0); //leftBlink
    }
  }
  
  bitWrite(kbusmsg[4],4,rearFogLight); //rearFog
  bitWrite(kbusmsg[4],3,frontFogLight); //frontFog
  bitWrite(kbusmsg[4],2,highBeam); //highbeam
  
  //shows only the car image, I have no idea on how to show the doors:
  //https://github.com/piersholt/wilhelm-docs/blob/master/gm/7a.md
  //https://github.com/tsharp42/E46ClusterDriver/blob/master/E46%20Documentation/Useful%20KBUS%20Codes.txt
  if(doors) {
    bitWrite(kbusmsg[7],7,true);
  } else {
    bitWrite(kbusmsg[7],7,false);
  }
  
  uint8_t cs = 0;
  for (uint8_t i = 0; i < 9; ++i) {
    cs ^= kbusmsg[i];
  }
  kbusmsg[9] = cs;
  Serial1.write(kbusmsg, sizeof(kbusmsg));
}

void BMWE46Cluster::setFuel(GameState& game) {
  int percentage = game.fuelQuantity;
  int pot1Percentage = (percentage > 50) ? percentage - 50 : 0;
  int pot2Percentage = (percentage < 50) ? percentage : 50;

  int desiredPositionPot1 = map(pot1Percentage, 0, 50, game.configuration.minimumFuelPotValue, game.configuration.maximumFuelPotValue);
  fuelPot.setPosition(desiredPositionPot1, false);
  
  int desiredPositionPot2 = map(pot2Percentage, 0, 50, game.configuration.minimumFuelPot2Value, game.configuration.maximumFuelPot2Value);
  fuelPot2.setPosition(desiredPositionPot2, false);
}

//____WORKING/DONE____
//rpm = done-works
//speed = done-works
//GearState gear/driveMode = done-works
//coolantTemperature = done-works with lamp
//absLight = done-works
//asc/offroad = done-works (even 2004+)
//rearFogLight,frontFogLight,highBeam,rightTurningIndicator,leftTurningIndicator = done-works
//fake l/100km gauge = done
//fuelQuantity = done, use values in the .h file
//handbrake = done-works
//backlight = works, 2004- with pin 7, 2004+ force set with kbus

//____NOT WORTH IT?_____
//batteryLight,turningIndicatorsBlinking,outdoorTemperature,ignition,backlightBrightness,mainLights

//1st is connector of IKE, second is ESP/potentiometer pin
//1&2 outside temperature
//9 CAN+ ; 10 CAN-
//11 black:TANK1+ (black/red/white) = A2 ; 12 = TANK1- (Brown/black/white) = A1
//15 black:TANK2+ (black/red/yellow) = B1 ; 16 = TANK2- (Brown/black/yellow) = B2
//14 black (white/red/yellow) = 17 KBUS (shift)
//19 black (yellow/green) = 22 speed (shift)
//22 black (black/yellow) = 21 abs (shift)
//23 black (blue/brown/yellow) = 13 handbrake (shift)
//7 black (grey/red) = 12v backlight
//4,5,6 black = 12V
//24,26,20 black & 4 white = gnd