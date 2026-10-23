// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

// load of barnacles (https://github.com/SkodaSuperbB6), 20.09.2026

// Do not touch GMLAN ID 0x10EC8040 !!
// It is presumably tied to the vehicle's VIN number, odometer shows dashes after sending this

#include "OpelCorsaECluster.h"

OpelCorsaECluster::OpelCorsaECluster(MCP_CAN& CAN, uint32_t configuredMileage, bool odometerEnabled, int diagTestMode, int languageSet) : CAN(CAN) {
  this->configuredMileage = configuredMileage;
  this->odometerEnabled = odometerEnabled;
  this->diagTestMode = diagTestMode;
  this->languageSet = languageSet;
}

float OpelCorsaECluster::mapSpeed(GameState& game) {
  float scaledSpeed = game.speed * game.configuration.speedCorrectionFactor;
  if (scaledSpeed > game.configuration.maximumSpeedValue) {
    return game.configuration.maximumSpeedValue;
  } else {
    return scaledSpeed;
  }
}

int OpelCorsaECluster::mapRPM(GameState& game) {
  int scaledRPM = game.rpm * game.configuration.rpmCorrectionFactor;
  if (scaledRPM > game.configuration.maximumRPMValue) {
    return game.configuration.maximumRPMValue;
  } else {
    return scaledRPM;
  }
}

int OpelCorsaECluster::mapCoolantTemperature(GameState& game) {
  if (game.coolantTemperature < game.configuration.minimumCoolantTemperature) { return game.configuration.minimumCoolantTemperature; }
  if (game.coolantTemperature > game.configuration.maximumCoolantTemperature) { return game.configuration.maximumCoolantTemperature; }
  return game.coolantTemperature;
}

void OpelCorsaECluster::updateWithGame(GameState& game) {
  if (millis() - lastDashboardUpdateTime250ms >= dashboardUpdateTime250) {
    sendFuel(game.fuelQuantity, mapRPM(game), game.configuration.maximumRPMValue);
    sendCoolant(mapCoolantTemperature(game));

    lastDashboardUpdateTime250ms = millis();
  }

  if (millis() - lastDashboardUpdateTime1000ms >= dashboardUpdateTime1000) {
    sendOdometer(game.speed);

    lastDashboardUpdateTime1000ms = millis();
  }

  if (millis() - lastDashboardUpdateTime3000ms >= dashboardUpdateTime3000) {
    sendGmlan();

    lastDashboardUpdateTime3000ms = millis();
  }

  if (millis() - lastDashboardUpdateTime >= dashboardUpdateTime50) {
    sendSpeed(mapSpeed(game));
    sendRPM(mapRPM(game));
    sendABS();
    sendTCS();
    sendLights(game.leftTurningIndicator, game.rightTurningIndicator, game.turningIndicatorsBlinking, game.mainLights, game.highBeam, game.rearFogLight);
    sendBacklight(game.backlightBrightness);
    sendHandbrake(game.handbrake);
    sendTPMS();

    sendDoorStatus(
      game.doorOpen, // Door front left
      false, // Door front right
      false, // Door rear left
      false, // Door rear right
      false, // Trunk
      false // Hood
    );

    sendEngine(
      false // Check engine light
    );

    sendAirbag(
      false // Seatbelt light
    );

    sendDiagTest();
    sendLanguage();

    seq++;
    if (seq > 15) {
      seq = 0;
    }

    lastDashboardUpdateTime = millis();
  }
}

void OpelCorsaECluster::sendSpeed(float speed) {
  unsigned char speedBuffer[8] = { 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00 },
                wheelBuffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  // Calculate to mph for vehicle speed
  float mph = speed * 0.621371f;
  uint16_t vehicleRaw = (uint16_t)(mph * 100.0f + 0.5f);

  // Calculate to yd/s for wheel speed
  float yds = (speed / 3.6f) * 1.0936133f;
  uint16_t wheelRaw = (uint16_t)(yds * 100.0f + 0.5f);

  // Vehicle speed
  speedBuffer[0] = vehicleRaw >> 8;
  speedBuffer[1] = vehicleRaw & 0xFF;
  speedBuffer[4] = vehicleRaw >> 8;
  speedBuffer[5] = vehicleRaw & 0xFF;

  // Wheel front left
  wheelBuffer[0] = wheelRaw >> 8;
  wheelBuffer[1] = wheelRaw & 0xFF;

  // Wheel rear left
  wheelBuffer[2] = wheelRaw >> 8;
  wheelBuffer[3] = wheelRaw & 0xFF;
  
  // Wheel front right
  wheelBuffer[4] = wheelRaw >> 8;
  wheelBuffer[5] = wheelRaw & 0xFF;

  // Wheel rear right
  wheelBuffer[6] = wheelRaw >> 8;
  wheelBuffer[7] = wheelRaw & 0xFF;

  CAN.sendMsgBuf(0x10210040, 1, 8, speedBuffer);
  CAN.sendMsgBuf(0x106B8040, 1, 8, wheelBuffer);
}

void OpelCorsaECluster::sendRPM(int rpm) {
  unsigned char rpmBuffer[8] = { 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 },
                transBuffer[8] = { 0x00, 0x00, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x00 };

  rpmBuffer[2] = (uint8_t)(rpm * 4 >> 8);    
  rpmBuffer[3] = (uint8_t)(rpm * 4 & 0x00FF);

  CAN.sendMsgBuf(0x102CA040, 1, 8, rpmBuffer);
  CAN.sendMsgBuf(0x102CC040, 1, 8, transBuffer);
}

void OpelCorsaECluster::sendABS() {
  unsigned char absBuffer[7] = { 0x00, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00 };

  CAN.sendMsgBuf(0x1022A040, 1, 7, absBuffer);
}

void OpelCorsaECluster::sendTCS() {
  unsigned char tcsBuffer[8] = { 0x83, 0xFA, 0x88, 0x01, 0xFF, 0x31, 0x00, 0x00 };

  CAN.sendMsgBuf(0x10240040, 1, 8, tcsBuffer);
}

void OpelCorsaECluster::sendDoorStatus(bool doorOpen, bool doorOpenFr, bool doorOpenRl, bool doorOpenRr, bool trunkOpen, bool hoodOpen) {
   unsigned char doorBuffer[1] = { 0x00 },
                 doorFrBuffer[1] = { 0x00 },
                 doorRlBuffer[1] = { 0x00 },
                 doorRrBuffer[1] = { 0x00 },
                 trunkBuffer[1] = { 0x00 },
                 hoodBuffer[1] = { 0x00 };

   uint8_t door_open = doorOpen ? 0x01 : 0;
   uint8_t door_open_fr = doorOpenFr ? 0x01 : 0;
   uint8_t door_open_rl = doorOpenRl ? 0x01 : 0;
   uint8_t door_open_rr = doorOpenRr ? 0x01 : 0;
   uint8_t trunk_open = trunkOpen ? 0x01 : 0;
   uint8_t hood_open = hoodOpen ? 0x02 : 0;

   doorBuffer[0] = door_open;
   doorFrBuffer[0] = door_open_fr;
   doorRlBuffer[0] = door_open_rl;
   doorRrBuffer[0] = door_open_rr;
   trunkBuffer[0] = trunk_open;
   hoodBuffer[0] = hood_open;

   CAN.sendMsgBuf(0x10630000, 1, 1, doorBuffer);
   CAN.sendMsgBuf(0x0C2F6040, 1, 1, doorFrBuffer);
   CAN.sendMsgBuf(0x0C2F8040, 1, 1, doorRlBuffer);
   CAN.sendMsgBuf(0x0C2FA040, 1, 1, doorRrBuffer);
   CAN.sendMsgBuf(0x106AA040, 1, 1, trunkBuffer);
   CAN.sendMsgBuf(0x10728040, 1, 1, hoodBuffer);
}

void OpelCorsaECluster::sendLights(bool leftBlinker, bool rightBlinker, bool blinkersBlinking, bool lowBeam, bool highBeam, bool rearFog) {
  unsigned char lightsBuffer[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

  int temp_turning_lights = 0 | (leftBlinker ? B00100000 : 0) | (rightBlinker ? B00000010 : 0);
  if (blinkersBlinking == true) {
    turning_lights_counter = turning_lights_counter + 1;
    if (turning_lights_counter <= 8) {
    } else if (turning_lights_counter > 8 && turning_lights_counter < 16) {
      temp_turning_lights = 0;
    } else {
      turning_lights_counter = 0;
    }
  }

  int low_beam_lights = lowBeam ? B01000000 : 0;
  int high_beam_lights = highBeam ? B00100000 : 0;
  int rear_fog_lights = rearFog ? B00010000 : 0;

  lightsBuffer[1] = low_beam_lights + high_beam_lights + rear_fog_lights;
  lightsBuffer[3] = temp_turning_lights;

  CAN.sendMsgBuf(0x1020C040, 1, 5, lightsBuffer);
}

void OpelCorsaECluster::sendBacklight(int brightness) {
  unsigned char backlightBuffer[3] = { 0x02, 0x00, 0x00 };

  uint8_t backlight_brightness = (uint8_t)((brightness * 255 + 50) / 100);

  backlightBuffer[1] = backlight_brightness;
  backlightBuffer[2] = backlight_brightness;

  CAN.sendMsgBuf(0x10644040, 1, 3, backlightBuffer);
}

void OpelCorsaECluster::sendFuel(int fuelAmount, int rpm, int configMaxRpm) {
  unsigned char fuelBuffer[6] = { 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00 },
                fuelRangeBuffer[8] = { 0x00, 0x00, 0x01, 0xA0, 0xFF, 0x00, 0x00, 0xFF },
                fuelConsumptionBuffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                fuelMessageBuffer[5] = { 0x00, 0x00, 0x00, 0x00, 0x14 };

  const int maxRPM = configMaxRpm;
  const uint8_t maxConsumption = 0xFF;

  // Fake consumption based on RPM
  uint8_t consumption = (uint8_t)((rpm * maxConsumption) / maxRPM);

  fuelBuffer[1] = (uint8_t)((fuelAmount * 255 + 50) / 100);
  fuelConsumptionBuffer[7] = consumption;

  CAN.sendMsgBuf(0x10800040, 1, 6, fuelBuffer);
  CAN.sendMsgBuf(0x102D0040, 1, 8, fuelRangeBuffer);
  CAN.sendMsgBuf(0x10264040, 1, 8, fuelConsumptionBuffer);
  CAN.sendMsgBuf(0x10806040, 1, 5, fuelMessageBuffer);
}

void OpelCorsaECluster::sendCoolant(int coolantTemperature) {
  unsigned char coolantBuffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  coolantBuffer[7] = (uint8_t)((coolantTemperature * 136) / 90);
  CAN.sendMsgBuf(0x102E0040, 1, 8, coolantBuffer);
}

void OpelCorsaECluster::sendEngine(bool checkEngine) {
  unsigned char engineBuffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00 };

  engineBuffer[2] = checkEngine ? 0 : 0x01;
  CAN.sendMsgBuf(0x10780040, 1, 8, engineBuffer);
}

void OpelCorsaECluster::sendHandbrake(bool handbrake) {
  unsigned char handbrakeBuffer[1] = { 0x04 };

  handbrakeBuffer[0] = handbrake ? 0x04 : 0;
  CAN.sendMsgBuf(0x103B4040, 1, 1, handbrakeBuffer);
}

void OpelCorsaECluster::sendAirbag(bool seatbelt) {
  unsigned char airbagBuffer[6] = { 0x73, 0x73, 0x73, 0x00, 0x00, 0x00 };

  airbagBuffer[0] = (uint8_t)((seatbelt ? 0x40 : 0) + 0x73);
  CAN.sendMsgBuf(0x10330058, 1, 6, airbagBuffer);
}

void OpelCorsaECluster::sendTPMS() {
  unsigned char tireLearnBuffer[2] = { 0x05, 0x85 },
                tirePressureBuffer[6] = { 0x24, 0x24, 0x38, 0x38, 0x38, 0x38 };

  CAN.sendMsgBuf(0x103DC040, 1, 2, tireLearnBuffer);
  CAN.sendMsgBuf(0x103D4040, 1, 6, tirePressureBuffer);
}

void OpelCorsaECluster::initializeOdometer() {
  if (odometerInitialized)
      return;

  if (!odometerEnabled)
  {
      odometerInitialized = true;
      return;
  }

  odometerPrefs.begin("odo", false);

  uint32_t storedMileage = odometerPrefs.getUInt("mileage", 0);
  uint32_t storedSetting = odometerPrefs.getUInt("setting", 0xFFFFFFFF);

  // Check if odometer setting got changed or not
  if (storedSetting != configuredMileage)
  {
      // Load new configured mileage on cluster
      savedMileage = configuredMileage;

      odometerPrefs.putUInt("mileage", savedMileage);
      odometerPrefs.putUInt("setting", configuredMileage);
  }
  else
  {
      // Load last saved mileage from ESP32 on cluster
      savedMileage = storedMileage;
  }

  odometerRaw = savedMileage * 64;

  odometerPrefs.end();

  lastMillis = millis();
  lastSavedMileage = savedMileage;

  odometerInitialized = true;
}

void OpelCorsaECluster::sendOdometer(float speed) {
  if (!odometerInitialized)
  {
      initializeOdometer();
  }

  if (!odometerEnabled)
  {
      return;
  }

  uint32_t currentMillis = millis();

  uint32_t deltaTime = currentMillis - lastMillis;
  lastMillis = currentMillis;

  float meters = (speed / 3.6f) * (deltaTime / 1000.0f);

  distanceMeters += (uint32_t)meters;

  // Save mileage for every 1 km
  if (distanceMeters >= 1000)
  {
      uint32_t kmAdded = distanceMeters / 1000;

      savedMileage += kmAdded;

      distanceMeters %= 1000;

      odometerPrefs.begin("odo", false);

      odometerPrefs.putUInt("mileage", savedMileage);

      odometerPrefs.end();
  }

  uint32_t displayMileage =
      (savedMileage * 1000) + distanceMeters;

  // Mileage on cluster incremented at 0.1 km
  uint32_t odometerRaw = (uint32_t)(
      (displayMileage * 64ULL) / 1000ULL
  );

  unsigned char odometerBuffer[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

  odometerBuffer[0] = (odometerRaw >> 24) & 0xFF;
  odometerBuffer[1] = (odometerRaw >> 16) & 0xFF;
  odometerBuffer[2] = (odometerRaw >> 8) & 0xFF;
  odometerBuffer[3] = odometerRaw & 0xFF;

  CAN.sendMsgBuf(0x106C0040, 1, 5, odometerBuffer);
}

void OpelCorsaECluster::sendDiagTest() {
  unsigned char diagExitBuffer[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                diagNeedleBuffer[8] = { 0x03, 0xAE, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 },
                diagChristmasBuffer[8] = { 0x03, 0xAE, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00 },
                diagDisplayBuffer[8] = { 0x03, 0xAE, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00 };

  switch (diagTestMode)
  {
      case 0:
          CAN.sendMsgBuf(0x24C, 0, 8, diagExitBuffer);
          break;

      case 1:
          // Needle sweep for all gauges
          CAN.sendMsgBuf(0x24C, 0, 8, diagNeedleBuffer);
          break;

      case 2:
          // All indicators & warning lights on
          CAN.sendMsgBuf(0x24C, 0, 8, diagChristmasBuffer);
          break;

      case 3:
          // White display screen
          CAN.sendMsgBuf(0x24C, 0, 8, diagDisplayBuffer);
          break;

      default:
          diagTestMode = 0;
          break;
  }
}

void OpelCorsaECluster::sendLanguage() {
  unsigned char languageBuffer[1] = { 0x00 };

  switch (languageSet)
  {
      case 0:
          // English
          languageBuffer[0] = { 0x00 };
          CAN.sendMsgBuf(0x10730080, 1, 1, languageBuffer);
          break;

      case 1:
          // German
          languageBuffer[0] = { 0x01 };
          CAN.sendMsgBuf(0x10730080, 1, 1, languageBuffer);
          break;

      case 2:
          // Italian
          languageBuffer[0] = { 0x02 };
          CAN.sendMsgBuf(0x10730080, 1, 1, languageBuffer);
          break;

      case 3:
          // Swedish
          languageBuffer[0] = { 0x03 };
          CAN.sendMsgBuf(0x10730080, 1, 1, languageBuffer);
          break;

      case 4:
          // French
          languageBuffer[0] = { 0x04 };
          CAN.sendMsgBuf(0x10730080, 1, 1, languageBuffer);
          break;

      case 5:
          // Spanish
          languageBuffer[0] = { 0x05 };
          CAN.sendMsgBuf(0x10730080, 1, 1, languageBuffer);
          break;

      case 6:
          // Dutch
          languageBuffer[0] = { 0x06 };
          CAN.sendMsgBuf(0x10730080, 1, 1, languageBuffer);
          break;

      default:
          languageSet = 0;
          break;
  }
}

void OpelCorsaECluster::sendGmlan() {
  // Keep alive message
  unsigned char gmlanEmptyBuffer[1] = { 0 };

  CAN.sendMsgBuf(0x100, 0, 0, gmlanEmptyBuffer);
}

void OpelCorsaECluster::sendTest() {
   // unsigned char testBuffer[1] = { 0x00 }; 

   // CAN.sendMsgBuf(0x10730080, 1, 1, testBuffer); 
}