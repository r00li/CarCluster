// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

// load of barnacles (https://github.com/SkodaSuperbB6), 06.08.2026

#include "OpelCorsaECluster.h"

OpelCorsaECluster::OpelCorsaECluster(MCP_CAN& CAN, uint32_t configuredMileage, bool odometerEnabled, int diagTestMode) : CAN(CAN) {
  this->configuredMileage = configuredMileage;
  this->odometerEnabled = odometerEnabled;
  this->diagTestMode = diagTestMode;
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

    seq++;
    if (seq > 15) {
      seq = 0;
    }

    lastDashboardUpdateTime = millis();
  }
}

void OpelCorsaECluster::sendSpeed(float speed) {
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

  CAN.sendMsgBuf(SPEED_ID, 1, 8, speedBuffer);
  CAN.sendMsgBuf(WHEEL_ID, 1, 8, wheelBuffer);
}

void OpelCorsaECluster::sendRPM(int rpm) {
  rpmBuffer[2] = (uint8_t)(rpm * 4 >> 8);    
  rpmBuffer[3] = (uint8_t)(rpm * 4 & 0x00FF);

  CAN.sendMsgBuf(RPM_ID, 1, 8, rpmBuffer);
  CAN.sendMsgBuf(TRANS_ID, 1, 8, transBuffer);
}

void OpelCorsaECluster::sendABS() {
  CAN.sendMsgBuf(ABS_ID, 1, 7, absBuffer);
}

void OpelCorsaECluster::sendTCS() {
  CAN.sendMsgBuf(TCS_ID, 1, 8, tcsBuffer);
}

void OpelCorsaECluster::sendDoorStatus(bool doorOpen, bool doorOpenFr, bool doorOpenRl, bool doorOpenRr, bool trunkOpen, bool hoodOpen) {
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

   CAN.sendMsgBuf(DOOR_ID, 1, 1, doorBuffer);
   CAN.sendMsgBuf(DOOR_FR_ID, 1, 1, doorFrBuffer);
   CAN.sendMsgBuf(DOOR_RL_ID, 1, 1, doorRlBuffer);
   CAN.sendMsgBuf(DOOR_RR_ID, 1, 1, doorRrBuffer);
   CAN.sendMsgBuf(TRUNK_ID, 1, 1, trunkBuffer);
   CAN.sendMsgBuf(HOOD_ID, 1, 1, hoodBuffer);
}

void OpelCorsaECluster::sendLights(bool leftBlinker, bool rightBlinker, bool blinkersBlinking, bool lowBeam, bool highBeam, bool rearFog) {
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

  CAN.sendMsgBuf(LIGHTS_ID, 1, 5, lightsBuffer);
}

void OpelCorsaECluster::sendBacklight(int brightness) {
  uint8_t backlight_brightness = (uint8_t)((brightness * 255 + 50) / 100);

  backlightBuffer[1] = backlight_brightness;
  backlightBuffer[2] = backlight_brightness;

  CAN.sendMsgBuf(BACKLIGHT_ID, 1, 3, backlightBuffer);
}

void OpelCorsaECluster::sendFuel(int fuelAmount, int rpm, int configMaxRpm) {
  const int maxRPM = configMaxRpm;
  const uint8_t maxConsumption = 0xFF;

  // Fake consumption based on RPM
  uint8_t consumption = (uint8_t)((rpm * maxConsumption) / maxRPM);

  fuelBuffer[1] = (uint8_t)((fuelAmount * 255 + 50) / 100);
  fuelConsumptionBuffer[7] = consumption;

  CAN.sendMsgBuf(FUEL_ID, 1, 6, fuelBuffer);
  CAN.sendMsgBuf(FUEL_RANGE_ID, 1, 8, fuelRangeBuffer);
  CAN.sendMsgBuf(FUEL_MESSAGE_ID, 1, 5, fuelMessageBuffer);
  CAN.sendMsgBuf(FUEL_CONSUMPTION_ID, 1, 8, fuelConsumptionBuffer);
}

void OpelCorsaECluster::sendCoolant(int coolantTemperature) {
  coolantBuffer[7] = (uint8_t)((coolantTemperature * 136) / 90);
  CAN.sendMsgBuf(COOLANT_ID, 1, 8, coolantBuffer);
}

void OpelCorsaECluster::sendEngine(bool checkEngine) {
  engineBuffer[2] = checkEngine ? 0 : 0x01;
  CAN.sendMsgBuf(ENGINE_ID, 1, 8, engineBuffer);
}

void OpelCorsaECluster::sendHandbrake(bool handbrake) {
  handbrakeBuffer[0] = handbrake ? 0x04 : 0;
  CAN.sendMsgBuf(HANDBRAKE_ID, 1, 1, handbrakeBuffer);
}

void OpelCorsaECluster::sendAirbag(bool seatbelt) {
  airbagBuffer[0] = (uint8_t)((seatbelt ? 0x40 : 0) + 0x73);
  CAN.sendMsgBuf(AIRBAG_ID, 1, 6, airbagBuffer);
}

void OpelCorsaECluster::sendTPMS() {
  CAN.sendMsgBuf(TIRE_LEARN_ID, 1, 2, tireLearnBuffer);
  CAN.sendMsgBuf(TIRE_PRESSURE_ID, 1, 6, tirePressureBuffer);
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

  odometerBuffer[0] = (odometerRaw >> 24) & 0xFF;
  odometerBuffer[1] = (odometerRaw >> 16) & 0xFF;
  odometerBuffer[2] = (odometerRaw >> 8) & 0xFF;
  odometerBuffer[3] = odometerRaw & 0xFF;

  CAN.sendMsgBuf(ODOMETER_ID, 1, 5, odometerBuffer);
}

void OpelCorsaECluster::sendDiagTest() {
  switch (diagTestMode)
  {
      case 0:
          CAN.sendMsgBuf(DIAG_ID, 0, 8, diagExitBuffer);
          break;

      case 1:
          // Needle sweep for all gauges
          CAN.sendMsgBuf(DIAG_ID, 0, 8, diagNeedleBuffer);
          break;

      case 2:
          // All indicators & warning lights on
          CAN.sendMsgBuf(DIAG_ID, 0, 8, diagChristmasBuffer);
          break;

      case 3:
          // White display screen
          CAN.sendMsgBuf(DIAG_ID, 0, 8, diagDisplayBuffer);
          break;

      default:
          diagTestMode = 0;
          break;
  }
}

void OpelCorsaECluster::sendGmlan() {
  // Keep alive message
  CAN.sendMsgBuf(GMLAN_ID, 0, 0, gmlanEmptyBuffer);
}

void OpelCorsaECluster::sendTest() {
   // unsigned char testBuffer[3] = { 0x02, 0x00, 0x00 }; 

   // CAN.sendMsgBuf(BACKLIGHT_ID, 1, 3, testBuffer); 
}