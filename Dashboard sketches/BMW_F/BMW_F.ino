// ####################################################################################################################
//
// CarCluster
// BMW F series Edition
// https://github.com/r00li/CarCluster
//
// By Andrej Rolih
// https://www.r00li.com
//
// ####################################################################################################################

// Libraries
#include <mcp_can.h>  // CAN Bus Shield Compatibility Library (install through library manager: mcp_can by coryjfowler - tested using 1.5.0)
#include <SPI.h>      // CAN Bus Shield SPI Pin Library (arduino system library)
#include <AsyncUDP.h>     // For game integration (system library part of ESP core)
#include <ArduinoJson.h>  // For parsing serial data nad for ESPDash (install through library manager: ArduinoJson by Benoit Blanchon - tested using 6.20.1)
#include "X9C10X.h"       // For fuel level simulation (install through library manager: X9C10X by Rob Tillaart - tested using 0.2.2)
#include "MultiMap.h"     // For fuel level calculation - supports non linear mapping found on BMW clusters (install through library manager)

#include "WiFiManager.h"        // For easier wifi management (install through library manager: WiFiManager by tzapu - tested using 2.0.16-rc.2)
#include <WiFi.h>               // Arduino system library (part of ESP core)
#include <AsyncTCP.h>           // Requirement for ESP-DASH (install manually from: https://github.com/me-no-dev/AsyncTCP )
#include <ESPAsyncWebServer.h>  // Requirement for ESP-DASH (install manually from:  https://github.com/me-no-dev/ESPAsyncWebServer )
#include <ESPDash.h>            // Web dashboard ( install from library manager: ESP-DASH by Ayush Sharma - tested using 4.0.1)

#include "FSeriesDash.h"


// User configuration
// In order to connect to wifi the ESP will on first boot create a wifi access point called CarCluster. Connect to it (password is "cluster"),
// then open your web browser and navigate to 192.168.4.1 and use the UI there to connect your wifi network.
// If wifi is not connected after 3 minutes the ESP will continue normal operation and you can use it in Simgub/serial mode
const int forzaUDPPort = 1101;                           // UDP Port to listen to for Forza Motorsport. Configure Forza to send telemetry data to this port
const int beamNGUDPPort = 1102;                          // UDP Port to listen to for Beam NG. Configure Beam to send telemetry data to this port
const unsigned long webDashboardUpdateInterval = 2000;   // How often is the web dashboard updated
const float speedCorrectionFactor = 1.00;   // Calibration of speed gauge
const float rpmCorrectionFactor = 1.00;     // Calibration of RPM gauge
const int maximumRPMValue = 6000;           // Set what is the maximum RPM on your cluster
const int maximumSpeedValue = 260;          // Set what is the maximum speed on your cluster (in km/h)
#define IS_CAR_MINI 1                       // Is the car an F series based Mini? Used to decide how to calculate fuel amount

// Non linear fuel range for the car. Used to calculate the fuel indicator values
#if IS_CAR_MINI
  uint8_t inFuelRange[] = {0, 50, 100};
  uint8_t outFuelRange[] = {22, 7, 3};
  boolean isCarMini = true;
#else
  uint8_t inFuelRange[] = {0, 50, 100};
  uint8_t outFuelRange[] = {37, 18, 4};
  boolean isCarMini = false;
#endif


// Pin configuration
const int SPI_CS_PIN = 5;
const int CAN_INT = 2;

// Default values for dashboard parameters
// Use these variables to set what you want to see on the dashboard
int speed = 0;                           // Car speed in km/h
int rpm = 0;                             // Set the rev counter
uint8_t backlight_brightness = 100;      // Backlight brightness 0-99
boolean leftTurningIndicator = false;    // Left blinker
boolean rightTurningIndicator = false;   // Right blinker
boolean mainLights = true;               // Are the main lights turned on?
uint8_t gear = 0;                        // The gear that the car is in: 0 = clear, 1-9 = M1-M9, 10 = P, 11 = R, 12 = N, 13 = D
int coolantTemperature = 100;            // Coolant temperature 50-130C
int fuelQuantity = 100;                  // Amount of fuel
boolean handbrake = false;               // Enables handbrake signal
boolean rearFogLight = false;            // Enable rear Fog Light indicator
boolean frontFogLight = false;           // Enable front fog light indicator
boolean highBeam = false;                // Enable High Beam Light
boolean doorOpen = false;                // Simulate open doors
boolean ignition = true;                 // Ignition status (set to false for accessory)
boolean signal_offroad = false;          // Simulates Offroad drive mode
uint8_t driveMode = 3;                   // Current drive mode: 1= Traction, 2= Comfort+, 4= Sport, 5= Sport+, 6= DSC off, 7= Eco pro 

// TODO: Find the CAN IDs for some of these variables
boolean signal_abs = false;              // Shows ABS Signal on dashboard
boolean battery_warning = false;         // Show Battery Warning.

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

FSeriesDash fSeriesDash(CAN);

// Dyna HTML configuration
AsyncWebServer server(80);
ESPDash dashboard(&server);


Card speedCard(&dashboard, SLIDER_CARD, "Speed", "km/h", 0, maximumSpeedValue);
Card rpmCard(&dashboard, SLIDER_CARD, "RPM", "rpm", 0, maximumRPMValue);
Card fuelCard(&dashboard, SLIDER_CARD, "Fuel qunaity", "%", 0, 100);
Card highBeamCard(&dashboard, BUTTON_CARD, "High beam");
Card fogLampCard(&dashboard, BUTTON_CARD, "Rear Fog lamp");
Card frontFogLampCard(&dashboard, BUTTON_CARD, "Front Fog lamp");
Card leftTurningIndicatorCard(&dashboard, BUTTON_CARD, "Indicator left");
Card rightTurningIndicatorCard(&dashboard, BUTTON_CARD, "Indicator right");
Card mainLightsCard(&dashboard, BUTTON_CARD, "Main lights");
Card doorOpenCard(&dashboard, BUTTON_CARD, "Door open warning");
Card dscActiveCard(&dashboard, BUTTON_CARD, "DSC active");
Card gearCard(&dashboard, SLIDER_CARD, "Selected gear", "", 0, 13);
Card backlightCard(&dashboard, SLIDER_CARD, "Backlight brightness", "%", 0, 100);
Card coolantTemperatureCard(&dashboard, SLIDER_CARD, "Coolant temperature", "C", 0, 200);
Card handbrakeCard(&dashboard, BUTTON_CARD, "Handbrake");
Card buttonUpCard(&dashboard, BUTTON_CARD, "Steering button");
Card ignitionCard(&dashboard, BUTTON_CARD, "Ignition");
Card driveModeCard(&dashboard, SLIDER_CARD, "Drive mode", "", 1, 7);

// TEsting only. To be removed
/*
Card val0Card(&dashboard, SLIDER_CARD, "VAL0", "", 0, 255);
Card val1Card(&dashboard, SLIDER_CARD, "VAL1", "", 0, 255);
Card val2Card(&dashboard, SLIDER_CARD, "VAL2", "", 0, 255);
Card val3Card(&dashboard, SLIDER_CARD, "VAL3", "", 0, 255);
Card val4Card(&dashboard, SLIDER_CARD, "VAL4", "", 0, 255);
Card val5Card(&dashboard, SLIDER_CARD, "VAL5", "", 0, 255);
Card val6Card(&dashboard, SLIDER_CARD, "VAL6", "", 0, 255);
Card val7Card(&dashboard, SLIDER_CARD, "VAL7", "", 0, 255);
uint8_t val0 = 0, val1 = 0, val2 = 0, val3 = 0, val4 = 0, val5 = 0, val6 = 0, val7 = 0;
*/


// UDP server configuration (for telemetry data)
AsyncUDP forzaUdp;
AsyncUDP beamUdp;

// Serial JSON parsing
StaticJsonDocument<400> doc;

void setup() {
  // Define the outputs
  pinMode(SPI_CS_PIN, OUTPUT);
  pinMode(CAN_INT, INPUT);

  //Begin with Serial Connection
  Serial.begin(115200);

  // Connect to wifi
  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  bool res = wm.autoConnect("CarCluster", "carcluster");
  if(!res) {
      Serial.println("Wifi Failed to connect");
  } else {
      Serial.println("Wifi connected...yeey :)");
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
    highBeam = (bool)value;
    highBeamCard.update(value);
    dashboard.sendUpdates();
  });

  fogLampCard.attachCallback([&](int value) {
    rearFogLight = (bool)value;
    fogLampCard.update(value);
    dashboard.sendUpdates();
  });

  frontFogLampCard.attachCallback([&](int value) {
    frontFogLight = (bool)value;
    frontFogLampCard.update(value);
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

  mainLightsCard.attachCallback([&](int value) {
    mainLights = (bool)value;
    mainLightsCard.update(value);
    dashboard.sendUpdates();
  });

  doorOpenCard.attachCallback([&](int value) {
    doorOpen = (bool)value;
    doorOpenCard.update(value);
    dashboard.sendUpdates();
  });

  dscActiveCard.attachCallback([&](int value) {
    signal_offroad = (bool)value;
    dscActiveCard.update(value);
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

  handbrakeCard.attachCallback([&](int value) {
    handbrake = (bool)value;
    handbrakeCard.update(value);
    dashboard.sendUpdates();
  });

  buttonUpCard.attachCallback([&](int value) {
    buttonUpCard.update(value);
    dashboard.sendUpdates();
    fSeriesDash.sendSteeringWheelButton();
  });

  ignitionCard.attachCallback([&](int value) {
    ignition = (bool)value;
    ignitionCard.update(value);
    dashboard.sendUpdates();
  });

  driveModeCard.attachCallback([&](int value) {
    driveMode = value;
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

  updateWebDashboard();
}

// Send CAN Command (short version)
void CanSend(short address, byte a, byte b, byte c, byte d, byte e, byte f, byte g, byte h, byte length = 8) {
  unsigned char DataToSend[8] = { a, b, c, d, e, f, g, h };
  CAN.sendMsgBuf(address, 0, length, DataToSend);
}

void loop() {
  fSeriesDash.updateWithState(speed*speedCorrectionFactor,
                       rpm*rpmCorrectionFactor,
                         fuelQuantity,
                         inFuelRange,
                         outFuelRange,
                         isCarMini,
                         backlight_brightness,
                         leftTurningIndicator,
                         rightTurningIndicator,
                         gear,
                         coolantTemperature,
                         handbrake,
                         highBeam,
                         mainLights,
                         rearFogLight,
                         frontFogLight,
                         doorOpen,
                         signal_offroad,
                         ignition,
                         driveMode);

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
    highBeamCard.update(highBeam);
    fogLampCard.update(rearFogLight);
    frontFogLampCard.update(frontFogLight);
    leftTurningIndicatorCard.update(leftTurningIndicator);
    rightTurningIndicatorCard.update(rightTurningIndicator);
    mainLightsCard.update(mainLights);
    doorOpenCard.update(doorOpen);
    dscActiveCard.update(signal_offroad);
    gearCard.update(gear);
    backlightCard.update(backlight_brightness);
    coolantTemperatureCard.update(coolantTemperature);
    handbrakeCard.update(handbrake);
    ignitionCard.update(ignition);
    driveModeCard.update(driveMode);
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
      } else if (action == 11) {
        //mqbDash.sendTestBuffers();
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
          doorOpen = true;  // We are in a menu
        } else {
          doorOpen = false;
        }

        if (max_rpm > maximumRPMValue) {
          rpm = map(rpm, 0, max_rpm, 0, maximumRPMValue);
        }

        // SPEED
        memcpy(rpmBuff, (packet.data() + 256), 4);
        int someSpeed = *((float*)rpmBuff);
        someSpeed = someSpeed * 3.6;
        if (someSpeed > maximumSpeedValue) { someSpeed = maximumSpeedValue; }
        speed = someSpeed;

        // GEAR
        memcpy(rpmBuff, (packet.data() + 319), 1);
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
        memcpy(rpmBuff, (packet.data() + 318), 1);
        int handbrake = (int)(rpmBuff[0]);
        handbrake = handbrake > 0 ? true : false;
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
        memcpy(rpmBuff, (packet.data() + 10), 1);
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
        memcpy(rpmBuff, (packet.data() + 12), 4);
        int someSpeed = *((float*)rpmBuff);
        someSpeed = someSpeed * 3.6;               // Speed is in m/s
        if (someSpeed > maximumSpeedValue) { someSpeed = maximumSpeedValue; }  // Cap the speed
        speed = someSpeed;

        // CURRENT_ENGINE_RPM
        memcpy(rpmBuff, (packet.data() + 16), 4);
        rpm = *((float*)rpmBuff);
        if (rpm > maximumRPMValue) { rpm = maximumRPMValue; }  // Cap the RPM to 8000 since the cluster doesn't go higher

        // ENGINE TEMPERATURE
        memcpy(rpmBuff, (packet.data() + 24), 4);
        coolantTemperature = *((float*)rpmBuff);
        if (coolantTemperature < 50) { coolantTemperature = 50; }
        if (coolantTemperature > 150) { coolantTemperature = 150; }

        // LIGHTS
        memcpy(rpmBuff, (packet.data() + 44), 4);
        int lights = *((int*)rpmBuff);

        rightTurningIndicator = ((lights & 0x0040) != 0);
        leftTurningIndicator = ((lights & 0x0020) != 0);
        highBeam = ((lights & 0x0002) != 0);
        battery_warning = ((lights & 0x0200) != 0);
        signal_abs = ((lights & 0x0400) != 0);
        handbrake = ((lights & 0x0004) != 0);
        signal_offroad = ((lights & 0x0010) != 0);
      }
    });
  }
}

void decodeSimhub() {
  rpm = doc["rpm"];
  int max_rpm = doc["mrp"];
  if (max_rpm > maximumRPMValue) {
    rpm = map(rpm, 0, max_rpm, 0, maximumRPMValue);
  }

  const char* simGear = doc["gea"];
  switch(simGear[0]) {
    case '1': gear = 1; break;
    case '2': gear = 2; break;
    case '3': gear = 3; break;
    case '4': gear = 4; break;
    case '5': gear = 5; break;
    case '6': gear = 6; break;
    case '7': gear = 7; break;
    case '8': gear = 8; break;
    case '9': gear = 9; break;
    case 'P': gear = 10; break;
    case 'R': gear = 11; break;
    case 'N': gear = 12; break;
    case 'D': gear = 13; break;
  }

  speed = doc["spe"];
  if (speed > maximumSpeedValue) { speed = maximumSpeedValue; }  // Cap the speed
  
  leftTurningIndicator = doc["lft"];
  rightTurningIndicator = doc["rit"];
  coolantTemperature = doc["oit"];
  doorOpen = (doc["pau"] != 0 || doc["run"] == 0);
  fuelQuantity = doc["fue"];
  handbrake = doc["hnb"];
  signal_abs = doc["abs"];
  signal_offroad = doc["tra"];
}
