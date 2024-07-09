// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef GAME_SIMULATION
#define GAME_SIMULATION

#include "Arduino.h"

struct ClusterConfiguration {
  float speedCorrectionFactor = 1.00;   // Calibration of speed gauge
  float rpmCorrectionFactor = 1.00;     // Calibration of RPM gauge
  int maximumRPMValue = 6000;           // Set what is the maximum RPM on your cluster
  int maximumSpeedValue = 260;          // Set what is the maximum speed on your cluster (in km/h)
  int minimumCoolantTemperature = 0;    // Set what is the minimum coolant temperature on your cluster (in C)
  int maximumCoolantTemperature = 200;  // Set what is the maximum coolant temperature on your cluster (in C)
  int minimumFuelPotValue = 18;         // Calibration of fuel pot - minimum value
  int maximumFuelPotValue = 83;         // Calibration of fuel pot - maximum value
  int minimumFuelPot2Value = 17;        // Calibration of fuel pot 2 - minimum value
  int maximumFuelPot2Value = 75;        // Calibration of fuel pot 2 - maximum value
  int isDualFuelPot = false;

  static ClusterConfiguration updatedFromDefaults(ClusterConfiguration current, float speedCorrectionFactor, float rpmCorrectionFactor, int maximumRPMValue, int maximumSpeedValue, int minimumCoolantTemperature, int maximumCoolantTemperature, int minimumFuelPotValue, int maximumFuelPotValue, int minimumFuelPot2Value, int maximumFuelPot2Value) {
    ClusterConfiguration newConfiguration;

    newConfiguration.speedCorrectionFactor = speedCorrectionFactor;
    newConfiguration.rpmCorrectionFactor = rpmCorrectionFactor;

    if (maximumRPMValue > 0) {
      newConfiguration.maximumRPMValue = maximumRPMValue;
    } else {
      newConfiguration.maximumRPMValue = current.maximumRPMValue;
    }

    if (maximumSpeedValue > 0) {
      newConfiguration.maximumSpeedValue = maximumSpeedValue;
    } else {
      newConfiguration.maximumSpeedValue = current.maximumSpeedValue;
    }

    if (minimumCoolantTemperature > 0) {
      newConfiguration.minimumCoolantTemperature = minimumCoolantTemperature;
    } else {
      newConfiguration.minimumCoolantTemperature = current.minimumCoolantTemperature;
    }

    if (maximumCoolantTemperature > 0) {
      newConfiguration.maximumCoolantTemperature = maximumCoolantTemperature;
    } else {
      newConfiguration.maximumCoolantTemperature = current.maximumCoolantTemperature;
    }

    if (minimumFuelPotValue > -1) {
      newConfiguration.minimumFuelPotValue = minimumFuelPotValue;
    } else {
      newConfiguration.minimumFuelPotValue = current.minimumFuelPotValue;
    }

    if (maximumFuelPotValue > -1) {
      newConfiguration.maximumFuelPotValue = maximumFuelPotValue;
    } else {
      newConfiguration.maximumFuelPotValue = current.maximumFuelPotValue;
    }

    if (minimumFuelPot2Value > -1) {
      newConfiguration.minimumFuelPot2Value = minimumFuelPot2Value;
    } else {
      newConfiguration.minimumFuelPot2Value = current.minimumFuelPot2Value;
    }

    if (maximumFuelPot2Value > -1) {
      newConfiguration.maximumFuelPot2Value = maximumFuelPot2Value;
    } else {
      newConfiguration.maximumFuelPot2Value = current.maximumFuelPot2Value;
    }

    newConfiguration.isDualFuelPot = current.isDualFuelPot;

    return newConfiguration;
  }
};

enum GearState {
  GearState_Manual_1 = 1,
  GearState_Manual_2 = 2,
  GearState_Manual_3 = 3,
  GearState_Manual_4 = 4,
  GearState_Manual_5 = 5,
  GearState_Manual_6 = 6,
  GearState_Manual_7 = 7,
  GearState_Manual_8 = 8,
  GearState_Manual_9 = 9,
  GearState_Manual_10 = 10,
  GearState_Auto_P = 11,
  GearState_Auto_R = 12,
  GearState_Auto_N = 13,
  GearState_Auto_D = 14,
  GearState_Auto_S = 15
};

class GameState {
  public:
  // Configuration
  ClusterConfiguration configuration;

  // Main parameters
  int speed = 0;                                     // Car speed in km/h
  int rpm = 0;                                       // Set the rev counter
  enum GearState gear = GearState_Auto_P;            // The gear that the car is in
  uint8_t backlightBrightness = 100;                 // Backlight brightness 0-99
  int coolantTemperature = 100;                      // Coolant temperature 50-130C
  bool ignition = true;                              // Ignition status (set to false for accessory)
  int fuelQuantity = 100;                            // Amount of fuel
  int outdoorTemperature = 20;                       // Outdoor temperature (from -50 to 50)

  // Indicators
  bool leftTurningIndicator = false;                 // Left blinker
  bool rightTurningIndicator = false;                // Right blinker
  bool turningIndicatorsBlinking = false;             // Should blinking be controlled by the game or by this sketch?
  bool mainLights = true;                            // Are the main lights turned on?
  bool handbrake = false;                            // Enables handbrake signal
  bool rearFogLight = false;                         // Enable rear Fog Light indicator
  bool frontFogLight = false;                        // Enable front fog light indicator
  bool highBeam = false;                             // Enable High Beam Light
  bool doorOpen = false;                             // Simulate open doors
  bool offroadLight = false;                         // Simulates Offroad drive mode
  uint8_t driveMode = 3;                             // Current drive mode for BMW: 1= Traction, 2= Comfort+, 4= Sport, 5= Sport+, 6= DSC off, 7= Eco pro 
  bool absLight = false;                             // Shows ABS Signal on dashboard
  bool batteryLight = false;                         // Show Battery Warning.

  // Other stuff
  int buttonEventToProcess = 0;                      // Certain clusters have buttons that can perform actions. Set this to activate them - values are cluster dependent

  GameState(ClusterConfiguration configuration) {
    this->configuration = configuration;
  }
};

class Game {
  public:
  virtual void begin() = 0;

  protected:
  Game(GameState& game): gameState(game) {};
  GameState &gameState;
};

#endif