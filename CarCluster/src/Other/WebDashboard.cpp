// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "WebDashboard.h"

WebDashboard::WebDashboard(GameState &game, int serverPort, unsigned long webDashboardUpdateInterval):
  gameState(game),
  server(serverPort), 
  dashboard(&server),
  introCard(&dashboard, GENERIC_CARD, "Info"),
  speedCard(&dashboard, SLIDER_CARD, "Speed", "km/h", 0, gameState.configuration.maximumSpeedValue),
  rpmCard(&dashboard, SLIDER_CARD, "RPM", "rpm", 0, gameState.configuration.maximumRPMValue),
  fuelCard(&dashboard, SLIDER_CARD, "Fuel qunaity", "%", 0, 100),
  highBeamCard(&dashboard, BUTTON_CARD, "High beam"),
  fogLampCard(&dashboard, BUTTON_CARD, "Rear Fog lamp"),
  frontFogLampCard(&dashboard, BUTTON_CARD, "Front Fog lamp"),
  leftTurningIndicatorCard(&dashboard, BUTTON_CARD, "Indicator left"),
  rightTurningIndicatorCard(&dashboard, BUTTON_CARD, "Indicator right"),
  mainLightsCard(&dashboard, BUTTON_CARD, "Main lights"),
  doorOpenCard(&dashboard, BUTTON_CARD, "Door open warning"),
  dscActiveCard(&dashboard, BUTTON_CARD, "DSC active"),
  gearCard(&dashboard, SLIDER_CARD, "Selected gear", "", 1, 15),
  backlightCard(&dashboard, SLIDER_CARD, "Backlight brightness", "%", 0, 100),
  coolantTemperatureCard(&dashboard, SLIDER_CARD, "Coolant temperature", "C", gameState.configuration.minimumCoolantTemperature, gameState.configuration.maximumCoolantTemperature),
  handbrakeCard(&dashboard, BUTTON_CARD, "Handbrake"),
  button1Card(&dashboard, BUTTON_CARD, "Steering button 1"),
  button2Card(&dashboard, BUTTON_CARD, "Steering button 2"),
  button3Card(&dashboard, BUTTON_CARD, "Steering button 3"),
  ignitionCard(&dashboard, BUTTON_CARD, "Ignition"),
  driveModeCard(&dashboard, SLIDER_CARD, "[BMW] Drive mode", "", 1, 7) { 
  this->webDashboardUpdateInterval = webDashboardUpdateInterval;
  introCard.update("Not all functions are available on all clusters");
}

void WebDashboard::begin() {
  speedCard.attachCallback([&](int value) {
    gameState.speed = value;
    speedCard.update(value);
    dashboard.sendUpdates();
  });

  rpmCard.attachCallback([&](int value) {
    gameState.rpm = value;
    rpmCard.update(value);
    dashboard.sendUpdates();
  });

  fuelCard.attachCallback([&](int value) {
    gameState.fuelQuantity = value;
    fuelCard.update(value);
    dashboard.sendUpdates();
  });

  highBeamCard.attachCallback([&](int value) {
    gameState.highBeam = (bool)value;
    highBeamCard.update(value);
    dashboard.sendUpdates();
  });

  fogLampCard.attachCallback([&](int value) {
    gameState.rearFogLight = (bool)value;
    fogLampCard.update(value);
    dashboard.sendUpdates();
  });

  frontFogLampCard.attachCallback([&](int value) {
    gameState.frontFogLight = (bool)value;
    frontFogLampCard.update(value);
    dashboard.sendUpdates();
  });

  leftTurningIndicatorCard.attachCallback([&](int value) {
    gameState.leftTurningIndicator = (bool)value;
    leftTurningIndicatorCard.update(value);
    dashboard.sendUpdates();
  });

  rightTurningIndicatorCard.attachCallback([&](int value) {
    gameState.rightTurningIndicator = (bool)value;
    rightTurningIndicatorCard.update(value);
    dashboard.sendUpdates();
  });

  mainLightsCard.attachCallback([&](int value) {
    gameState.mainLights = (bool)value;
    mainLightsCard.update(value);
    dashboard.sendUpdates();
  });

  doorOpenCard.attachCallback([&](int value) {
    gameState.doorOpen = (bool)value;
    doorOpenCard.update(value);
    dashboard.sendUpdates();
  });

  dscActiveCard.attachCallback([&](int value) {
    gameState.offroadLight = (bool)value;
    dscActiveCard.update(value);
    dashboard.sendUpdates();
  });

  gearCard.attachCallback([&](int value) {
    gameState.gear = static_cast<GearState>(value);
    gearCard.update(value);
    dashboard.sendUpdates();
  });

  backlightCard.attachCallback([&](int value) {
    gameState.backlightBrightness = value;
    backlightCard.update(value);
    dashboard.sendUpdates();
  });

  coolantTemperatureCard.attachCallback([&](int value) {
    gameState.coolantTemperature = value;
    coolantTemperatureCard.update(value);
    dashboard.sendUpdates();
  });

  handbrakeCard.attachCallback([&](int value) {
    gameState.handbrake = (bool)value;
    handbrakeCard.update(value);
    dashboard.sendUpdates();
  });

  button1Card.attachCallback([&](int value) {
    button1Card.update(value);
    dashboard.sendUpdates();
    gameState.buttonEventToProcess = 1;
  });

  button2Card.attachCallback([&](int value) {
    button2Card.update(value);
    dashboard.sendUpdates();
    gameState.buttonEventToProcess = 2;
  });

  button3Card.attachCallback([&](int value) {
    button3Card.update(value);
    dashboard.sendUpdates();
    gameState.buttonEventToProcess = 3;
  });

  ignitionCard.attachCallback([&](int value) {
    gameState.ignition = (bool)value;
    ignitionCard.update(value);
    dashboard.sendUpdates();
  });

  driveModeCard.attachCallback([&](int value) {
    gameState.driveMode = value;
    driveModeCard.update(value);
    dashboard.sendUpdates();
  });

  /*
  val0Card.attachCallback([&](int value) {
    val0 = value;
    val0Card.update(value);
    dashboard.sendUpdates();
  });

  val1Card.attachCallback([&](int value) {
    val1 = value;
    val1Card.update(value);
    dashboard.sendUpdates();
  });

  val2Card.attachCallback([&](int value) {
    val2 = value;
    val2Card.update(value);
    dashboard.sendUpdates();
  });

  val3Card.attachCallback([&](int value) {
    val3 = value;
    val3Card.update(value);
    dashboard.sendUpdates();
  });

  val4Card.attachCallback([&](int value) {
    val4 = value;
    val4Card.update(value);
    dashboard.sendUpdates();
  });

  val5Card.attachCallback([&](int value) {
    val5 = value;
    val5Card.update(value);
    dashboard.sendUpdates();
  });

  val6Card.attachCallback([&](int value) {
    val6 = value;
    val6Card.update(value);
    dashboard.sendUpdates();
  });

  val7Card.attachCallback([&](int value) {
    val7 = value;
    val7Card.update(value);
    dashboard.sendUpdates();
  });
*/

  server.begin();
  update();
}

void WebDashboard::update() {
  // Prevent too frequent updates of the web dashboard
  if (millis() - lastWebDashboardUpdateTime >= webDashboardUpdateInterval) {
    speedCard.update(gameState.speed);
    rpmCard.update(gameState.rpm);
    fuelCard.update(gameState.fuelQuantity);
    highBeamCard.update(gameState.highBeam);
    fogLampCard.update(gameState.rearFogLight);
    frontFogLampCard.update(gameState.frontFogLight);
    leftTurningIndicatorCard.update(gameState.leftTurningIndicator);
    rightTurningIndicatorCard.update(gameState.rightTurningIndicator);
    mainLightsCard.update(gameState.mainLights);
    doorOpenCard.update(gameState.doorOpen);
    dscActiveCard.update(gameState.offroadLight);
    gearCard.update(gameState.gear);
    backlightCard.update(gameState.backlightBrightness);
    coolantTemperatureCard.update(gameState.coolantTemperature);
    handbrakeCard.update(gameState.handbrake);
    ignitionCard.update(gameState.ignition);
    driveModeCard.update(gameState.driveMode);
    dashboard.sendUpdates();

    lastWebDashboardUpdateTime = millis();
  }
}
