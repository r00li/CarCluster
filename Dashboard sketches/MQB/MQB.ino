// ####################################################################################################################
//
// CarCluster
// MQB (VW T-Cross) Edition
// https://github.com/r00li/CarCluster
//
// By Andrej Rolih
// https://www.r00li.com
//
// MQB CAN message implementation based on work by Ronaldo Cordeiro ( https://github.com/ronaldocordeiro )
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

#include "MQBDash.h"


// User configuration
// In order to connect to wifi the ESP will on first boot create a wifi access point called CarCluster. Connect to it (password is "cluster"),
// then open your web browser and navigate to 192.168.4.1 and use the UI there to connect your wifi network.
// If wifi is not connected after 3 minutes the ESP will continue normal operation and you can use it in Simgub/serial mode
const int forzaUDPPort = 1101;                           // UDP Port to listen to for Forza Motorsport. Configure Forza to send telemetry data to this port
const int beamNGUDPPort = 1102;                          // UDP Port to listen to for Beam NG. Configure Beam to send telemetry data to this port
const unsigned long webDashboardUpdateInterval = 2000;   // How often is the web dashboard updated
const int minimumFuelPotValue = 22;                      // Calibration of the fuel pot - minimum value
const int maximumFuelPotValue = 80;                      // Calibration of the fuel pot - maximum value

// Pin configuration
const int fuelPotInc = 14;
const int fuelPotDir = 27;
const int fuelPotCs = 12;
const int SPI_CS_PIN = 5;
const int CAN_INT = 2;

// Default values for dashboard parameters
// Use these variables to set what you want to see on the dashboard
int speed = 0;                           // Car speed in km/h
int rpm = 0;                             // Set the rev counter
uint8_t backlight_brightness = 99;       // Backlight brightness 0-99 (only valid when backlight == true)
boolean leftTurningIndicator = false;    // Left blinker
boolean rightTurningIndicator = false;   // Right blinker
boolean turning_lights_blinking = true;  // Choose the mode of the turning lights (blinking or just shining)
uint8_t gear = 0;                        // The gear that the car is in: 0 = clear, 1-9 = M1-M9, 10 = P, 11 = R, 12 = N, 13 = D
int coolantTemperature = 90;             // Coolant temperature 50-130C
int fuelQuantity = 100;                  // Amount of fuel

// TODO: Find the CAN IDs for some of these variables
boolean signal_abs = false;              // Shows ABS Signal on dashboard
boolean signal_offroad = false;          // Simulates Offroad drive mode
boolean signal_handbrake = false;        // Enables handbrake signal
boolean signal_lowtirepressure = false;  // Simulates low tire pressure
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


// Helper constants
const unsigned int MAX_SERIAL_MESSAGE_LENGTH = 250;

// Global variables for various dashboard functionallity
unsigned long lastWebDashboardUpdateTime = 0;

// CAN bus configuration
MCP_CAN CAN(SPI_CS_PIN);  // Set CS pin

// CAN bus Receiving
long unsigned int canRxId;
unsigned char canRxLen = 0;
unsigned char canRxBuf[8];
char canRxMsgString[128];  // Array to store serial string

// CAN bus devices
MQBDash mqbDash(CAN);

// Fuel level simulation
X9C102 fuelPot = X9C102();

// Dyna HTML configuration
AsyncWebServer server(80);
ESPDash dashboard(&server);

Card speedCard(&dashboard, SLIDER_CARD, "Speed", "km/h", 0, 260);
Card rpmCard(&dashboard, SLIDER_CARD, "RPM", "rpm", 0, 8000);
Card fuelCard(&dashboard, SLIDER_CARD, "Fuel qunaity", "%", 0, 100);
Card highBeamCard(&dashboard, BUTTON_CARD, "High beam");
Card fogLampCard(&dashboard, BUTTON_CARD, "Fog lamp");
Card leftTurningIndicatorCard(&dashboard, BUTTON_CARD, "Indicator left");
Card rightTurningIndicatorCard(&dashboard, BUTTON_CARD, "Indicator right");
Card indicatorsBlinkCard(&dashboard, BUTTON_CARD, "Indicators blink");
Card doorOpenCard(&dashboard, BUTTON_CARD, "Door open warning");
Card gearCard(&dashboard, SLIDER_CARD, "Selected gear", "", 0, 13);
Card backlightCard(&dashboard, SLIDER_CARD, "Backlight brightness", "%", 0, 99);
Card coolantTemperatureCard(&dashboard, SLIDER_CARD, "Coolant temperature", "C", 50, 130);

// TEsting only. To be removed
Card val0Card(&dashboard, SLIDER_CARD, "VAL0", "", 0, 255);
Card val1Card(&dashboard, SLIDER_CARD, "VAL1", "", 0, 255);
Card val2Card(&dashboard, SLIDER_CARD, "VAL2", "", 0, 255);
Card val3Card(&dashboard, SLIDER_CARD, "VAL3", "", 0, 255);
Card val4Card(&dashboard, SLIDER_CARD, "VAL4", "", 0, 255);
Card val5Card(&dashboard, SLIDER_CARD, "VAL5", "", 0, 255);
Card val6Card(&dashboard, SLIDER_CARD, "VAL6", "", 0, 255);
Card val7Card(&dashboard, SLIDER_CARD, "VAL7", "", 0, 255);
uint8_t val0 = 0, val1 = 0, val2 = 0, val3 = 0, val4 = 0, val5 = 0, val6 = 0, val7 = 0;

// UDP server configuration (for telemetry data)
AsyncUDP forzaUdp;
AsyncUDP beamUdp;

// Serial JSON parsing
StaticJsonDocument<400> doc;

void setup() {
  // Define the outputs
  pinMode(SPI_CS_PIN, OUTPUT);
  pinMode(CAN_INT, INPUT);

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
    indicatorsBlinkCard.update(value);
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

  coolantTemperatureCard.attachCallback([&](int value) {
    coolantTemperature = value;
    coolantTemperatureCard.update(value);
    dashboard.sendUpdates();
  });

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

  server.begin();

  updateWebDashboard();
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
  // Update the dashboard
  mqbDash.updateWithState(speed, rpm, backlight_brightness, leftTurningIndicator, rightTurningIndicator, turning_lights_blinking, gear, coolantTemperature);

  setFuel(fuelQuantity);

  //Testing only. To be removed
  //mqbDash.updateTestBuffer(val0, val1, val2, val3, val4, val5, val6, val7);

  // Serial message handling
  readSerialJson();

  // Handle data from connected CAN hardware
  readCanBuffer();

  // Update the web dashboard
  updateWebDashboard();
}

void updateWebDashboard() {
  // Prevent too frequent updates of the web dashboard
  if (millis() - lastWebDashboardUpdateTime >= webDashboardUpdateInterval) {
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
    coolantTemperatureCard.update(coolantTemperature);
    dashboard.sendUpdates();

    lastWebDashboardUpdateTime = millis();
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
        // Used to decode custom protocol from Simhub in the following format:
        // {"action":10, "spe":54, "gea":"2", "rpm":3590, "mrp":7999, "lft":0, "rit":0, "oit":0, "pau":0, "run":0, "fue":0, "hnb":0, "abs":0, "tra":0}
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
        if (someSpeed > 260) { someSpeed = 260; }
        speed = someSpeed;

        // GEAR
        mempcpy(rpmBuff, (packet.data() + 319), 1);
        int forzaGear = (int)(rpmBuff[0]);
        if (forzaGear == 0) {
          gear = 11;
        } else if (forzaGear > 9) {
          gear = 13;
        } else {
          gear = forzaGear;
        }
        if (max_rpm == 0) { gear = 10; }  // Idle

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
          gear = 11;
        } else if (beamGear == 1) {
          gear = 13;
        } else if (beamGear >= 9) {
          gear = 13;
        } else {
          gear = beamGear - 1;
        }

        // SPEED
        mempcpy(rpmBuff, (packet.data() + 12), 4);
        int someSpeed = *((float*)rpmBuff);
        someSpeed = someSpeed * 3.6;               // Speed is in m/s
        if (someSpeed > 260) { someSpeed = 260; }  // Cap the speed to 260 since the cluster doesn't go higher
        speed = someSpeed;

        // CURRENT_ENGINE_RPM
        memcpy(rpmBuff, (packet.data() + 16), 4);
        rpm = *((float*)rpmBuff);
        if (rpm > 8000) { rpm = 8000; }  // Cap the RPM to 8000 since the cluster doesn't go higher

        // ENGINE TEMPERATURE
        memcpy(rpmBuff, (packet.data() + 24), 4);
        coolantTemperature = *((float*)rpmBuff);
        if (coolantTemperature < 50) { coolantTemperature = 50; }
        if (coolantTemperature > 130) { coolantTemperature = 130; }

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
  coolantTemperature = doc["oit"];
  door_open = (doc["pau"] != 0 || doc["run"] == 0);
  fuelQuantity = doc["fue"];
  signal_handbrake = doc["hnb"];
  signal_abs = doc["abs"];
  signal_offroad = doc["tra"];
}
