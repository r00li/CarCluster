// ####################################################################################################################
//
// CarCluster
// Polo 6r edition
// https://github.com/r00li/CarCluster
//
// By Andrej Rolih
// https://www.r00li.com
//
// Based on Volkswagen CAN BUS Gaming by Leon Bataille ( https://hackaday.io/project/6288-volkswagen-can-bus-gaming )
//
// ####################################################################################################################

// Libraries
#include <mcp_can.h>  // CAN Bus Shield Compatibility Library (install through library manager: mcp_can by coryjfowler - tested using 1.5.0)
#include <SPI.h>      // CAN Bus Shield SPI Pin Library (arduino system library)
#include <AsyncUDP.h>     // For game integration (system library part of ESP core)
#include <ArduinoJson.h>  // For parsing serial data nad for ESPDash (install through library manager: ArduinoJson by Benoit Blanchon - tested using 6.20.1)
#include "X9C10X.h"       // For fuel level simulation (install through library manager: X9C10X by Rob Tillaart - tested using 0.2.2)

#include "WiFiManager.h"        // For easier wifi management (install through library manager: WiFiManager by tzapu - tested using 2.0.16-rc.2)
#include <WiFi.h>               // Arduino system library (part of ESP core)
#include <AsyncTCP.h>           // Requirement for ESP-DASH (install manually from: https://github.com/me-no-dev/AsyncTCP )
#include <ESPAsyncWebServer.h>  // Requirement for ESP-DASH (install manually from:  https://github.com/me-no-dev/ESPAsyncWebServer )
#include <ESPDash.h>            // Web dashboard ( install from library manager: ESP-DASH by Ayush Sharma - tested using 4.0.1)



// User configuration
const int forzaUDPPort = 1101;             // UDP Port to listen to for Forza Motorsport. Configure Forza to send telemetry data to this port
const int beamNGUDPPort = 1102;            // UDP Port to listen to for Beam NG. Configure Beam to send telemetry data to this port
const int updateClusterDelayMs = 5;        // At least how much time (in ms) must pass before we send the data to cluster again. Set to 0 for as fast as possible. Should be no faster than 10ms - causes cluster to be overwhelmed.
const int minimumFuelPotValue = 18;        // Calibration of the fuel pot - minimum value
const int maximumFuelPotValue = 85;        // Calibration of the fuel pot - maximum value

// Pin configuration
const int sprinklerWaterSensor = 4;
const int coolantShortageSensor = 16;
const int oilPressureSwitch = 15;
const int fuelPotInc = 14;
const int fuelPotDir = 27;
const int fuelPotCs = 12;
const int SPI_CS_PIN = 5;
const int CAN_INT = 2;

// Default values for dashboard parameters
// Use these variables to set what you want to see on the dashboard
int speed = 0;                           // Car speed in km/h
int rpm = 0;                             // Set the rev counter
boolean backlight = true;                // Turn the automatic dashboard backlight on or off
uint8_t backlight_brightness = 99;       // Backlight brightness 0-99 (only valid when backlight == true)
boolean leftTurningIndicator = false;    // Left blinker
boolean rightTurningIndicator = false;   // Right blinker
boolean turning_lights_blinking = true;  // Choose the mode of the turning lights (blinking or just shining)
boolean add_distance = false;            // Dashboard counts the kilometers (can't be undone)
int distance_multiplier = 2;             // Sets the refresh rate of the odometer (Standard 2)
boolean signal_abs = false;              // Shows ABS Signal on dashboard
boolean signal_offroad = false;          // Simulates Offroad drive mode
boolean signal_handbrake = false;        // Enables handbrake signal
boolean signal_lowtirepressure = false;  // Simulates low tire pressure
boolean oil_pressure_simulation = true;  // Set this to true if dashboard starts to beep
boolean door_open = false;               // Simulate open doors
boolean clutch_control = false;          // Displays the Message "Kupplung" (German for Clutch) on the dashboard's LCD
boolean check_lamp = false;              // Show 'Check Lamp' Signal on dashboard. B00010000 = on, B00000 = off
boolean trunklid_open = false;           // Simulate open trunk lid (Kofferraumklappe). B00100000 = open, B00000 = closed
boolean battery_warning = false;         // Show Battery Warning.
boolean keybattery_warning = false;      // Show message 'Key Battery Low' on Display. But just after first start of dashboard.
boolean light_fog = false;               // Enable Fog Light indicator
boolean light_highbeam = false;          // Enable High Beam Light
boolean seat_belt = false;               // Switch Seat Betl warning light.
boolean signal_dieselpreheat = false;    // Simualtes Diesel Preheating
boolean signal_watertemp = false;        // Simualtes high water temperature
boolean dpf_warning = false;             // Shows the Diesel particle filter warning signal.
int fuelQuantity = 100;                  // Amount of fuel
uint8_t gear = 0;                        // The gear that the car is in: 0 = P, 1-7 = Gear 1-7, 8 = R, 9 = N, 10 = D

// Helper constants
#define lo8(x) ((int)(x)&0xff)
#define hi8(x) ((int)(x) >> 8)

const unsigned int MAX_SERIAL_MESSAGE_LENGTH = 250;

// Global variables for various dashboard functionallity
int turning_lights_counter = 0;
int distance_counter = 0;
uint8_t iterationCount = 0;
unsigned long lastDashboardUpdateTime = 0;

// CAN bus configuration
MCP_CAN CAN(SPI_CS_PIN);  // Set CS pin

// CAN bus Receiving
long unsigned int canRxId;
unsigned char canRxLen = 0;
unsigned char canRxBuf[8];
char canRxMsgString[128];  // Array to store serial string

// Fuel level simulation
X9C102 fuelPot = X9C102();

// Dyna HTML configuration
AsyncWebServer server(80);
ESPDash dashboard(&server);

Card speedCard(&dashboard, SLIDER_CARD, "Speed", "km/h", 0, 240);
Card rpmCard(&dashboard, SLIDER_CARD, "RPM", "rpm", 0, 8000);
Card fuelCard(&dashboard, SLIDER_CARD, "Fuel qunaity", "%", 0, 100);
Card highBeamCard(&dashboard, BUTTON_CARD, "High beam");
Card fogLampCard(&dashboard, BUTTON_CARD, "Fog lamp");
Card leftTurningIndicatorCard(&dashboard, BUTTON_CARD, "Indicator left");
Card rightTurningIndicatorCard(&dashboard, BUTTON_CARD, "Indicator right");
Card indicatorsBlinkCard(&dashboard, BUTTON_CARD, "Indicators blink");
Card doorOpenCard(&dashboard, BUTTON_CARD, "Door open warning");
Card gearCard(&dashboard, SLIDER_CARD, "Selected gear", "", 0, 10);
Card backlightCard(&dashboard, SLIDER_CARD, "Backlight brightness", "%", 0, 99);

// UDP server configuration (for telemetry data)
AsyncUDP forzaUdp;
AsyncUDP beamUdp;

// Serial JSON parsing
StaticJsonDocument<400> doc;

void setup() {
  // Define the outputs
  pinMode(sprinklerWaterSensor, OUTPUT);
  pinMode(coolantShortageSensor, OUTPUT);
  pinMode(oilPressureSwitch, OUTPUT);

  pinMode(SPI_CS_PIN, OUTPUT);
  pinMode(CAN_INT, INPUT);

  digitalWrite(sprinklerWaterSensor, HIGH);
  digitalWrite(coolantShortageSensor, HIGH);
  digitalWrite(oilPressureSwitch, LOW);

  fuelPot.begin(fuelPotInc, fuelPotDir, fuelPotCs);
  fuelPot.setPosition(100, true); // Force the pot to a known value

  //Begin with Serial Connection
  Serial.begin(115200);

  // Connect to wifi
  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  bool res = wm.autoConnect("CarCluster", "cluster");
  if(!res) {
      Serial.println("Wifi Failed to connect");
  } else {
      Serial.println("Wifi cvonnected...yeey :)");
  }
  
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  setupWebPage();
  listenForzaUDP(forzaUDPPort);
  listenBeamNGUDP(beamNGUDPPort);

  //Begin with CAN Bus Initialization
START_INIT:
  if (CAN_OK == CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ))  // init can bus : baudrate = 500k
  {
    Serial.println("CAN BUS Shield init ok!");
  } else {
    Serial.println("CAN BUS Shield init fail");
    Serial.println("Init CAN BUS Shield again");
    delay(100);
    goto START_INIT;
  }
  CAN.setMode(MCP_NORMAL);
}

void setupWebPage() {
  speedCard.attachCallback([&](int value) {
    speed = value;
    speedCard.update(value);
    dashboard.sendUpdates();
  });

  rpmCard.attachCallback([&](int value) {
    rpm = value;
    rpmCard.update(value);
    dashboard.sendUpdates();
  });

  fuelCard.attachCallback([&](int value) {
    fuelQuantity = value;
    fuelCard.update(value);
    dashboard.sendUpdates();
  });

  highBeamCard.attachCallback([&](int value) {
    light_highbeam = (bool)value;
    highBeamCard.update(value);
    dashboard.sendUpdates();
  });

  fogLampCard.attachCallback([&](int value) {
    light_fog = (bool)value;
    fogLampCard.update(value);
    dashboard.sendUpdates();
  });

  leftTurningIndicatorCard.attachCallback([&](int value) {
    leftTurningIndicator = (bool)value;
    leftTurningIndicatorCard.update(value);
    dashboard.sendUpdates();
  });

  rightTurningIndicatorCard.attachCallback([&](int value) {
    rightTurningIndicator = (bool)value;
    rightTurningIndicatorCard.update(value);
    dashboard.sendUpdates();
  });

  indicatorsBlinkCard.attachCallback([&](int value) {
    turning_lights_blinking = (bool)value;
    rightTurningIndicatorCard.update(value);
    dashboard.sendUpdates();
  });

  doorOpenCard.attachCallback([&](int value) {
    door_open = (bool)value;
    doorOpenCard.update(value);
    dashboard.sendUpdates();
  });

  gearCard.attachCallback([&](int value) {
    gear = value;
    gearCard.update(value);
    dashboard.sendUpdates();
  });

  backlightCard.attachCallback([&](int value) {
    backlight_brightness = value;
    backlightCard.update(value);
    dashboard.sendUpdates();
  });

  server.begin();
}

// Send CAN Command (short version)
void CanSend(short address, byte a, byte b, byte c, byte d, byte e, byte f, byte g, byte h, byte length = 8) {
  unsigned char DataToSend[8] = { a, b, c, d, e, f, g, h };
  CAN.sendMsgBuf(address, 0, length, DataToSend);
}

// Fuel gauge control (0-100%)
void setFuel(int percentage) {
  int desiredPosition = map(percentage, 0, 100, minimumFuelPotValue, maximumFuelPotValue);
  fuelPot.setPosition(desiredPosition, false);
}

void loop() {
  // Prepare the data as needed

  if (millis() - lastDashboardUpdateTime >= updateClusterDelayMs) {
    // This should probably be done using a more sophisticated method like a scheduler, but for now this seems to work.
    // This is done in order to not cause issues when sending the data too frequently.
    // If we are sending this data too frequently then the cluster will start rebooting randomly when showing things like ABS/parking brake/... lights

    // Oil pressure simulation
    if (oil_pressure_simulation == true) {
      //Set Oil Pressure Switch
      if (rpm > 1500) {
        digitalWrite(oilPressureSwitch, LOW);
      } else {
        digitalWrite(oilPressureSwitch, HIGH);
      }
    }

    // Turning Lights (blinkers)
    int turning_lights = 0;
    if (leftTurningIndicator == true && rightTurningIndicator == false) {
      turning_lights = 1;
    } else if (rightTurningIndicator == true && leftTurningIndicator == false) {
      turning_lights = 2;
    } else if (leftTurningIndicator == true && rightTurningIndicator == true) {
      turning_lights = 3;
    } else {
      turning_lights = 0;
    }

    int temp_turning_lights = 0;
    if (turning_lights_blinking == true) {
      turning_lights_counter = turning_lights_counter + 1;

      if (turning_lights_counter <= 40) {
        temp_turning_lights = turning_lights;
      } else if (turning_lights_counter > 40 && turning_lights_counter < 80) {
        temp_turning_lights = 0;
      } else {
        turning_lights_counter = 0;
      }
    } else {
      temp_turning_lights = turning_lights;
    }

    // DPF
    int temp_dpf_warning = 0;
    if (dpf_warning == true) {
      temp_dpf_warning = B00000010;
    } else {
      temp_dpf_warning = B00000000;
    }

    // Seat Belt
    int temp_seat_belt = 0;
    if (seat_belt == true) {
      temp_seat_belt = B00000100;
    } else {
      temp_seat_belt = B00000000;
    }

    // Battery Warning
    int temp_battery_warning = 0;
    if (battery_warning == true) {
      temp_battery_warning = B10000000;
    } else {
      temp_battery_warning = B00000000;
    }

    // Trunk Lid (Kofferraumklappe)
    int temp_trunklid_open = 0;
    if (trunklid_open == true) {
      temp_trunklid_open = B00100000;
    } else {
      temp_trunklid_open = B00000000;
    }

    // Check Lamp Signal
    int temp_check_lamp = 0;
    if (check_lamp == true) {
      temp_check_lamp = B00010000;
    } else {
      temp_check_lamp = B00000000;
    }

    // Clutch Text on LCD
    int temp_clutch_control = 0;
    if (clutch_control == true) {
      temp_clutch_control = B00000001;
    } else {
      temp_clutch_control = B00000000;
    }

    // Warning for low key battery
    int temp_keybattery_warning = 0;
    if (keybattery_warning == true) {
      temp_keybattery_warning = B10000000;
    } else {
      temp_keybattery_warning = B00000000;
    }

    // Lightmode Selection (Fog Light and/or High Beam)
    int temp_light_highbeam = 0;
    if (light_highbeam == true) {
      temp_light_highbeam = B01000000;
    } else {
      temp_light_highbeam = B00000000;
    }

    int temp_light_fog = 0;
    if (light_fog == true) {
      temp_light_fog = B00100000;
    } else {
      temp_light_fog = B00000000;
    }

    int lightmode = temp_light_highbeam + temp_light_fog;

    // Engine Options (Water Temperature, Diesel Preheater)
    int temp_signal_dieselpreheat = 0;
    if (signal_dieselpreheat == true) {
      temp_signal_dieselpreheat = B00000010;
    } else {
      temp_signal_dieselpreheat = B00000000;
    }

    int temp_signal_watertemp = 0;
    if (signal_watertemp == true) {
      temp_signal_watertemp = B00010000;
    } else {
      temp_signal_watertemp = B00000000;
    }

    int engine_control = temp_signal_dieselpreheat + temp_signal_watertemp;

    // Drivemode Selection (ABS, Offroad, Low Tire Pressure, handbrake)
    int temp_signal_abs = 0;
    if (signal_abs == true) {
      temp_signal_abs = B0001;
    } else {
      temp_signal_abs = B0000;
    }

    int temp_signal_offroad = 0;
    if (signal_offroad == true) {
      temp_signal_offroad = B0010;
    } else {
      temp_signal_offroad = B0000;
    }

    int temp_signal_handbrake = 0;
    if (signal_handbrake == true) {
      temp_signal_handbrake = B0100;
    } else {
      temp_signal_handbrake = B0000;
    }

    int temp_signal_lowtirepressure = 0;
    if (signal_lowtirepressure == true) {
      temp_signal_lowtirepressure = B1000;
    } else {
      temp_signal_lowtirepressure = B0000;
    }
    int drive_mode = temp_signal_abs + temp_signal_offroad + temp_signal_handbrake + temp_signal_lowtirepressure;

    // Prepare Speed
    int temp_speed = speed / 0.0070;  //KMH=1.12 MPH=0.62
    byte speedL = lo8(temp_speed);
    byte speedH = hi8(temp_speed);

    // Prepare RPM
    short tempRPM = rpm * (4 + (float)((float)(1 * 1600) / (float)rpm));
    byte rpmL = lo8(tempRPM);
    byte rpmH = hi8(tempRPM);

    // Prepare gear indicator
    uint tempGear = gear;
    switch (tempGear) {
      case 0: tempGear = 0; break;                                     // P
      case 1 ... 7: tempGear = 0x2E + ((tempGear - 1) * 0x10); break;  // 1-7
      case 8: tempGear = 0x36; break;                                  // R
      case 9: tempGear = 0x40; break;                                  // N
      case 10: tempGear = 0x56; break;                                 // D
    }

    // Prepare distance adder
    if (add_distance == true) {
      int distance_adder = temp_speed * distance_multiplier;
      distance_counter += distance_adder;
      if (distance_counter > distance_adder) { distance_counter = 0; }
    }

    // Send the prepared data

    //Immobilizer
    CanSend(0x3D0, 0, 0x80, 0, 0, 0, 0, 0, 0);

    // Engine on and ESP enabled
    // Random is required to keep the speed needle steady
    // After 63 it's not stable anymore
    // With temp1 = 4, speed=100 needle is solid... possibly temp1 is checksum
    uint8_t tempCounter = speed > 0 ? iterationCount : 0;
    uint8_t tempValue1 = (speed >= 63) ? random(0, 255) : 0;
    uint8_t tempValue2 = (speed >= 63) ? random(0, 255) : 0;
    CanSend(0xDA0, 0x01, speedL, speedH, 0x00, 0x00, tempCounter, tempValue1, tempValue2);

    //Enable Cruise Control
    //CanSend(0x289, 0x00, B00000001, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    //Turning Lights 2
    CanSend(0x470, temp_battery_warning + temp_turning_lights, temp_trunklid_open + door_open, (backlight ? backlight_brightness | 1 : 0), 0x00, temp_check_lamp + temp_clutch_control, temp_keybattery_warning, 0x00, lightmode);

    //Diesel engine
    CanSend(0x480, 0x00, engine_control, 0x00, 0x00, 0x00, temp_dpf_warning, 0x00, 0x00);

    ///Engine
    //CanSend(0x388, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    //Cruise Control
    //CanSend(0x289, 0x00, B00000101, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    //Motorspeed
    CanSend(0x320, 0x00, (speedL * 100), (speedH * 100), 0x00, 0x00, 0x00, 0x00, 0x00);

    //RPM
    CanSend(0x280, 0x49, 0x0E, rpmL, rpmH, 0x0E, 0x00, 0x1B, 0x0E);

    //Speed
    CanSend(0x5A0, 0xFF, speedL, speedH, drive_mode, 0x00, lo8(distance_counter), hi8(distance_counter), 0xad);

    //ABS
    CanSend(0x1A0, iterationCount, speedL, speedH, 0x34, 0xFE, 0xFE, 0x00, 0x04);

    //Airbag
    CanSend(0x050, 0x00, 0x80, temp_seat_belt, 0x00, 0x00, 0x00, 0x00, 0x00);

    // Last byte 56=D, 36=R, 40=N, 2E=1, 3E=2, 4E=3, 5E=4 6E=5, 7E=6, 8E=7
    CanSend(0x540, 0x90, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x02, tempGear);

    // Key inserted, car started, ignition on?
    CanSend(0x572, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F);

    setFuel(fuelQuantity);

    // Iteration counters used by a few CAN messages that are needed
    iterationCount++;

    lastDashboardUpdateTime = millis();
  }

  // Serial message handling
  readSerialJson();
  readCanBuffer();
  updateWebDashboard();
}

void updateWebDashboard() {
  // Prevent too frequent updates of the web dashboard
  if (iterationCount == 50 || iterationCount == 100 || iterationCount == 255) {
    speedCard.update(speed);
    rpmCard.update(rpm);
    fuelCard.update(fuelQuantity);
    highBeamCard.update(light_highbeam);
    fogLampCard.update(light_fog);
    leftTurningIndicatorCard.update(leftTurningIndicator);
    rightTurningIndicatorCard.update(rightTurningIndicator);
    indicatorsBlinkCard.update(turning_lights_blinking);
    doorOpenCard.update(door_open);
    gearCard.update(gear);
    backlightCard.update(backlight_brightness);
    dashboard.sendUpdates();
  }
}

void readSerialJson() {
  //Check to see if anything is available in the serial receive buffer
  while (Serial.available() > 0) {
    //Create a place to hold the incoming message
    static char message[MAX_SERIAL_MESSAGE_LENGTH];
    static unsigned int message_pos = 0;

    //Read the next available byte in the serial receive buffer
    char inByte = Serial.read();

    //Message coming in (check not terminating character) and guard for over message size
    if (inByte != '\n' && (message_pos < MAX_SERIAL_MESSAGE_LENGTH - 1)) {
      //Add the incoming byte to our message
      message[message_pos] = inByte;
      message_pos++;
    } else {
      //Add null character to string
      message[message_pos] = '\0';

      Serial.print("I got: @");
      Serial.print(message);
      Serial.println("@");

      DeserializationError error = deserializeJson(doc, message);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        message_pos = 0;
        return;
      }
      Serial.println(error.c_str());

      uint8_t action = doc["action"];

      // Action 0 means "send following message to CAN bus"
      // Example: {"action":0, "address":1644, "p1":128, "p2":20, "p3":76, "p4":85, "p5":9, "p6":66, "p7":108, "p8":117}
      if (action == 0) {
        short address = doc["address"];
        uint8_t p1 = doc["p1"];
        uint8_t p2 = doc["p2"];
        uint8_t p3 = doc["p3"];
        uint8_t p4 = doc["p4"];
        uint8_t p5 = doc["p5"];
        uint8_t p6 = doc["p6"];
        uint8_t p7 = doc["p7"];
        uint8_t p8 = doc["p8"];
        Serial.println(address);

        CanSend(address, p1, p2, p3, p4, p5, p6, p7, p8);
      } else if (action == 10) {
        decodeSimhub();
      }

      //Reset for the next message
      message_pos = 0;
    }
  }
}

void readCanBuffer() {
  if (!digitalRead(CAN_INT)) {                      // If CAN0_INT pin is low, read receive buffer
    CAN.readMsgBuf(&canRxId, &canRxLen, canRxBuf);  // Read data: len = data length, buf = data byte(s)

    // Uncomment if you want to see what is being received on the CAN bus
    /*
    if ((rxId & 0x80000000) == 0x80000000)  // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
    else
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);

    Serial.print(msgString);

    if ((rxId & 0x40000000) == 0x40000000) {  // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.print(msgString);
    } else {
      for (byte i = 0; i < len; i++) {
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        Serial.print(msgString);
      }
    }

    Serial.println();
    */
  }
}

void listenForzaUDP(int port) {
  // Forza (Horizon) sends data as a UDP blob of data
  // Telemetry protocol described here: https://medium.com/@makvoid/building-a-digital-dashboard-for-forza-using-python-62a0358cb43b

  if (forzaUdp.listen(port)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());

    forzaUdp.onPacket([](AsyncUDPPacket packet) {
      if (packet.length() == 324) {
        char rpmBuff[4];  // four bytes in a float 32

        // CURRENT_ENGINE_RPM
        memcpy(rpmBuff, (packet.data() + 16), 4);
        rpm = *((float*)rpmBuff);

        // IDLE_ENGINE_RPM
        //memcpy(rpmBuff, (packet.data() + 12), 4);
        //if (idle_rpm != *((float*)rpmBuff)) {
        //  idle_rpm = *((float*)rpmBuff);
        //}

        // MAX_ENGINE_RPM
        memcpy(rpmBuff, (packet.data() + 8), 4);
        float max_rpm = *((float*)rpmBuff);

        if (max_rpm == 0) {
          door_open = true;  // We are in a menu
        } else {
          door_open = false;
        }

        if (max_rpm > 8000) {
          rpm = map(rpm, 0, max_rpm, 0, 8000);
        }

        // SPEED
        mempcpy(rpmBuff, (packet.data() + 256), 4);
        int someSpeed = *((float*)rpmBuff);
        someSpeed = someSpeed * 3.6;
        if (someSpeed > 240) { someSpeed = 240; }
        speed = someSpeed;

        // GEAR
        mempcpy(rpmBuff, (packet.data() + 319), 1);
        int forzaGear = (int)(rpmBuff[0]);
        if (forzaGear == 0) {
          gear = 8;
        } else if (forzaGear > 7) {
          gear = 10;
        } else {
          gear = forzaGear;
        }
        if (max_rpm == 0) { gear = 0; }  // Idle

        // HANDBRAKE
        // For some reason keeping handbrake signal on causes weird issues with the cluster, keep it on for just a flash
        mempcpy(rpmBuff, (packet.data() + 318), 1);
        int handbrake = (int)(rpmBuff[0]);
        signal_handbrake = handbrake > 0 ? true : false;
      }
    });
  }
}

void listenBeamNGUDP(int port) {
  // Beam NG sends data as a UDP blob of data
  // Telemetry protocol described here: https://github.com/fuelsoft/out-gauge-cluster

  if (beamUdp.listen(port)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());

    beamUdp.onPacket([](AsyncUDPPacket packet) {
      if (packet.length() >= 64) {
        char rpmBuff[4];  // four bytes

        // GEAR
        mempcpy(rpmBuff, (packet.data() + 10), 1);
        int beamGear = (int)(0xFF & rpmBuff[0]);
        if (beamGear == 0) {
          gear = 8;
        } else if (beamGear == 1) {
          gear = 0;
        } else if (beamGear >= 9) {
          gear = 10;
        } else {
          gear = beamGear - 1;
        }

        // SPEED
        mempcpy(rpmBuff, (packet.data() + 12), 4);
        int someSpeed = *((float*)rpmBuff);
        someSpeed = someSpeed * 3.6;               // Speed is in m/s
        if (someSpeed > 240) { someSpeed = 240; }  // Cap the speed to 240 since the cluster doesn't go higher
        speed = someSpeed;

        // CURRENT_ENGINE_RPM
        memcpy(rpmBuff, (packet.data() + 16), 4);
        rpm = *((float*)rpmBuff);
        if (rpm > 8000) { rpm = 8000; }  // Cap the RPM to 8000 since the cluster doesn't go higher

        // LIGHTS
        mempcpy(rpmBuff, (packet.data() + 44), 4);
        int lights = *((int*)rpmBuff);

        turning_lights_blinking = false;  // Beam blinks the indicators itself
        rightTurningIndicator = ((lights & 0x0040) != 0);
        leftTurningIndicator = ((lights & 0x0020) != 0);
        light_highbeam = ((lights & 0x0002) != 0);
        battery_warning = ((lights & 0x0200) != 0);
        signal_abs = ((lights & 0x0400) != 0);
        signal_handbrake = ((lights & 0x0004) != 0);
        signal_offroad = ((lights & 0x0010) != 0);
      }
    });
  }
}

void decodeSimhub() {
  rpm = doc["rpm"];
  int max_rpm = doc["mrp"];
  if (max_rpm > 8000) {
    rpm = map(rpm, 0, max_rpm, 0, 8000);
  }

  const char* simGear = doc["gea"];
  switch(simGear[0]) {
    case 'P': gear = 0; break;
    case '1': gear = 1; break;
    case '2': gear = 2; break;
    case '3': gear = 3; break;
    case '4': gear = 4; break;
    case '5': gear = 5; break;
    case '6': gear = 6; break;
    case '7': gear = 7; break;
    case 'R': gear = 8; break;
    case 'N': gear = 9; break;
    case 'D': gear = 10; break;
  }

  speed = doc["spe"];
  leftTurningIndicator = doc["lft"];
  rightTurningIndicator = doc["rit"];
  int oilTemperature = doc["oit"]; // Not supported on Polo
  door_open = (doc["pau"] != 0 || doc["run"] == 0);
  fuelQuantity = doc["fue"];
  signal_handbrake = doc["hnb"];
  signal_abs = doc["abs"];
  signal_offroad = doc["tra"];
}

/*
// Some test data from a sniff of a more modern polo
if (enabledLines[0] == 1) { CanSend(0x1A0, 0x00, 0x40, 0x12, 0x34, 0xFE, 0xFE, 0x00, 0x04); }
if (enabledLines[1] == 1) { CanSend(0x4A0, 0xFC, 0x33, 0x28, 0x34, 0x4A, 0x34, 0x4A, 0x34); }
if (enabledLines[2] == 1) { CanSend(0x3A0, 0xDA, 0x02, 0x36, 0xC9, 0x1C, 0x70, 0x44, 0x0F); }
if (enabledLines[3] == 1) { CanSend(0x288, 0x8A, 0xB4, 0x30, 0x00, 0x51, 0x46, 0x9B, 0x09); }
if (enabledLines[4] == 1) { CanSend(0x480, 0x94, 0x00, 0x4D, 0xE1, 0x18, 0x00, 0x06, 0x26); }
if (enabledLines[5] == 1) { CanSend(0x48A, 0x7F, 0xDE, 0x01, 0x00, 0x00, 0x00, 0x00, 0xA0); }
if (enabledLines[6] == 1) { CanSend(0x588, 0xF0, 0x42, 0x7C, 0x00, 0x00, 0x00, 0x04, 0x00); }
if (enabledLines[7] == 1) { CanSend(0xD0, 0x94, 0x20, 0x04, 0x08, 0x08, 0xB0, 0x00, 0x00, 6); }
if (enabledLines[8] == 1) { CanSend(0x320, 0x04, 0x00, 0x19, 0x2B, 0x34, 0x83, 0x37, 0x80); }
if (enabledLines[9] == 1) { CanSend(0x440, 0x30, 0x58, 0x17, 0xFE, 0x79, 0x00, 0x4C, 0x13); }
if (enabledLines[10] == 1) { CanSend(0x540, 0x90, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x02, 0x56); }
if (enabledLines[11] == 1) { CanSend(0x280, 0x08, 0x2B, 0xA0, 0x16, 0x2B, 0x1E, 0x0B, 0x2D); }
if (enabledLines[12] == 1) { CanSend(0x380, 0x60, 0x6E, 0x24, 0x00, 0x00, 0x00, 0xFE, 0x08); }
if (enabledLines[13] == 1) { CanSend(0x488, 0x8B, 0x2D, 0x2A, 0x7C, 0xFE, 0x00, 0x3E, 0x30); }
if (enabledLines[14] == 1) { CanSend(0x50, 0x00, 0x30, 0x51, 0x61, 0x00, 0x00, 0x00, 0x00, 5); }
if (enabledLines[15] == 1) { CanSend(0x44C, 0x06, 0xA6, 0x45, 0x8A, 0x58, 0x02, 0xF5, 0xC0); }
if (enabledLines[16] == 1) { CanSend(0x3D0, 0x00, 0x40, 0x08, 0x00, 0xC8, 0x5F, 0x00, 0x00, 6); }
if (enabledLines[17] == 1) { CanSend(0x4A8, 0xFE, 0x7F, 0x00, 0x20, 0x00, 0x00, 0x20, 0x81); }
if (enabledLines[18] == 1) { CanSend(0x5A0, 0x7E, 0xEC, 0x26, 0x20, 0x00, 0x05, 0x0B, 0x2B); }
if (enabledLines[19] == 1) { CanSend(0x729, 0x04, 0x03, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 7); }
if (enabledLines[20] == 1) { CanSend(0x470, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0x1F); }
if (enabledLines[21] == 1) { CanSend(0x35F, 0x3A, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00); }
if (enabledLines[22] == 1) { CanSend(0x38A, 0xF1, 0x01, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 4); }
if (enabledLines[23] == 1) { CanSend(0x5D0, 0x80, 0x02, 0x50, 0x2F, 0x39, 0x59, 0x00, 0x00); }
if (enabledLines[24] == 1) { CanSend(0x570, 0x87, 0x00, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 5); }
if (enabledLines[25] == 1) { CanSend(0x5E0, 0x00, 0x00, 0x12, 0x02, 0x4B, 0x00, 0x00, 0x00); }
if (enabledLines[26] == 1) { CanSend(0x62B, 0x35, 0x10, 0x28, 0x40, 0x24, 0x00, 0x43, 0x40); }
if (enabledLines[27] == 1) { CanSend(0x77E, 0x04, 0x62, 0x22, 0x06, 0x15, 0xAA, 0xAA, 0xAA); }
*/