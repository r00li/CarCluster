// ####################################################################################################################
//
// CarCluster
// https://github.com/r00li/CarCluster
//
// By Andrej Rolih
// https://www.r00li.com
//
// ####################################################################################################################

// --------------------------------------------------------------------
// ----------------------- BEGIN USER CONFIGURATION -------------------
// --------------------------------------------------------------------

// To which Arduino/ESP pin have you connected the CS (chip select) pin of your CAN interface?
#define SPI_CS_PIN 5

// To which Arduino/ESP pin have you connected the INT (interrupt) pin of your CAN interface?
#define CAN_INT 2

// Select which cluster you will be using
// 1 = BMW F10, BMW F30 6WA (BMW F series)
// 2 = Mini F55 (BMW F series)
// 3 = Golf 7 (VW MQB)
// 4 = Polo 6R (VW PQ25)
// 5 = Skoda Superb 2 (VW PQ46)
// 99 = Golf 7 (VW MQB) - Passthrough mode (if using an external gateway, BCM, ignition lock, ...)
#define CLUSTER 1

// Configure the maximum RPM value shown on the cluster
// Leave at 0 for using the defaults based on the cluster. Change this is your cluster has different limits
#define MAXIMUM_RPM 0

// A correction factor for the RPM value. RPM will be multiplied by this value.
// This enables you to fix displayed values that are slightly off, though you might not be able
// to ever fully get them calibrated correctly - depending on the cluster. 
#define RPM_CORRECTION_FACTOR 1.0

// Configure the maximum speed shown on the cluster (in km/h)
// Leave at 0 for using the defaults based on the cluster. Change this is your cluster has different limits
#define MAXIMUM_SPEED 0

// A correction factor for the speed. Speed will be multiplied by this value.
// This enables you to fix displayed values that are slightly off, though you might not be able
// to ever fully get them calibrated correctly - depending on the cluster. 
#define SPEED_CORRECTION_FACTOR 1.0

// Define the minimum and maximum coolant temperature your cluster can display
// Leave alone if your cluster has no such display
// Leave at 0 for using the defaults based on the cluster. Change this is your cluster has different limits.
#define MINIMUM_COOLANT_TEMPERATURE 0
#define MAXIMUM_COOLANT_TEMPERATURE 0

// Select if you want Wifi to be enabled or not.
// Wifi gives you a web dashboard that you can use for testing
// and the ability to connect to certain games directly, but will only work on an ESP32. 
// If using a different board select 0.
//
// In order to connect to wifi the ESP will on first boot create a wifi access point called CarCluster. Connect to it (password is "carcluster"),
// then open your web browser and navigate to 192.168.4.1 and use the UI there to connect your wifi network (if configuration popup doesn't open automatically).
// If wifi is not connected after 3 minutes the ESP will continue normal operation and you can use it in Simhub/serial mode
// 
// 1 for enabled
// 0 for disabled
#define WIFI_ENABLED 1

// Analog fuel simulation - for clusters with separate fuel level pins (currently VW PQ and MQB clusters).
// Requires use of X9C10X digital potentiometer(s)
// If one or two potentiometers are used depends on the specific cluster.
// Only configure the values that make sense for your clusters - if using 1 pot leave configuration for pot 2 as is
//
// PIN CONFIGURATION:
#define ANALOG_FUEL_POT_INC 14
#define ANALOG_FUEL_POT_DIR 27
#define ANALOG_FUEL_POT_CS1 12
// If using dual fuel potentiometers (usually found on larger cars)
#define ANALOG_FUEL_POT_CS2 33
//
// CONFIGURATION OF MINIMUM AND MAXIMUM VALUES:
// Depends on speicific sluter. If fuel percentage indicated on your cluster is wrong, change these values
#define ANALOG_FUEL_POT_MINIMUM_VALUE 18
#define ANALOG_FUEL_POT_MAXIMUM_VALUE 83
#define ANALOG_FUEL_POT_MINIMUM_VALUE2 17
#define ANALOG_FUEL_POT_MAXIMUM_VALUE2 75

// VW PQ specific pin configuration (for various non CAN based values)
// If you are not using VW PQ based instrument cluster leave this alone
#define VWPQ_SPRINKLER_WATER_SENSOR_PIN 4
#define VWPQ_COOLANT_SHORTAGE_PIN 16
#define VWPQ_OIL_PRESSURE_SWITCH_PIN 15
#define VWPQ_HANDBRAKE_INDICATOR_PIN 13
#define VWPQ_BRAKE_FLUID_WARNING_PIN 22

// MQB Passthrough specific configuration
#define VWMQB_PASS_CAN2_CS 17
#define VWMQB_PASS_CAN2_INT 16

// --------------------------------------------------------------------
// ------------------------ END USER CONFIGURATION --------------------
// --------------------------------------------------------------------

// ------------------------ BEGIN OTHER CONFIGURATION -----------------
// Other configurable variables that usually don't need changing

// How often is the web dashboard updated
#define WIFI_WEB_DASHBOARD_UPDATE_INTERVAL 2000

// Name of the access point created
#define WIFI_CONFIG_PORTAL_ACCESS_POINT_NAME "CarCluster"

// Password for the configuration access point
#define WIFI_CONFIG_PORTAL_ACCESS_POINT_PASSWORD "carcluster"

// Timeout of the config portal after which it will continue in wifi-less mode
#define WIFI_CONFIG_PORTAL_TIMEOUT 180

// What is the maximum character length of the serial message
#define MAX_SERIAL_MESSAGE_LENGTH 250

// Baud rate of the USB serial connection (if using Simhub set this to the same value)
#define SERIAL_BAUD_RATE 921600

// UDP/TCP ports used for various games and other stuff.
// Only applicable if wifi is enabled.
#define WIFI_FORZA_UDP_PORT 1101
#define WIFI_BEAM_UDP_PORT 1102
#define WIFI_WEB_DASHBOARD_PORT 80

// ------------------------ END OTHER CONFIGURATION -------------------


// Libraries
#include <SPI.h> // CAN Bus Shield SPI Pin Library (arduino system library)

#include "src/Libs/ArduinoJson/ArduinoJson.h" // For parsing serial data and for ESPDash ( https://github.com/bblanchon/ArduinoJson )
#include "src/Libs/MCP_CAN/mcp_can.h" // CAN Bus Shield Compatibility Library ( https://github.com/coryjfowler/MCP_CAN_lib )

#include "src/Games/GameSimulation.h"
#include "src/Games/SimhubGame.h"

// CAN bus configuration
MCP_CAN CAN(SPI_CS_PIN);  // Set CS pin

// CAN bus Receiving
long unsigned int canRxId;
unsigned char canRxLen = 0;
unsigned char canRxBuf[8];
char canRxMsgString[128];  // Array to store serial string

// Cluster initialization
#if CLUSTER == 1
  // BMW F10
  #include "src/Clusters/BMW_F/BMWFSeriesCluster.h"
  BMWFSeriesCluster cluster(CAN, false);
  ClusterConfiguration defaultClusterConfig = cluster.clusterConfig(false);
#elif CLUSTER == 2
  // F Series Mini
  #include "src/Clusters/BMW_F/BMWFSeriesCluster.h"
  BMWFSeriesCluster cluster(CAN, true);
  ClusterConfiguration defaultClusterConfig = cluster.clusterConfig(true);
#elif CLUSTER == 3
  // Golf 7
  #include "src/Clusters/VW_MQB/VWMQBCluster.h"
  VWMQBCluster cluster(CAN, ANALOG_FUEL_POT_INC, ANALOG_FUEL_POT_DIR, ANALOG_FUEL_POT_CS1, ANALOG_FUEL_POT_CS2);
  ClusterConfiguration defaultClusterConfig = cluster.clusterConfig();
#elif CLUSTER == 4
  // VW Polo  6R
  #include "src/Clusters/VW_PQ25/VWPQ25Cluster.h"
  VWPQ25Cluster cluster(CAN, ANALOG_FUEL_POT_INC, ANALOG_FUEL_POT_DIR, ANALOG_FUEL_POT_CS1, ANALOG_FUEL_POT_CS2, VWPQ_SPRINKLER_WATER_SENSOR_PIN, VWPQ_COOLANT_SHORTAGE_PIN, VWPQ_OIL_PRESSURE_SWITCH_PIN, VWPQ_HANDBRAKE_INDICATOR_PIN, VWPQ_BRAKE_FLUID_WARNING_PIN);
  ClusterConfiguration defaultClusterConfig = cluster.clusterConfig();
#elif CLUSTER == 5
  // Skoda Superb 2
  #include "src/Clusters/VW_PQ46/VWPQ46Cluster.h"
  VWPQ46Cluster cluster(CAN, ANALOG_FUEL_POT_INC, ANALOG_FUEL_POT_DIR, ANALOG_FUEL_POT_CS1, ANALOG_FUEL_POT_CS2, VWPQ_SPRINKLER_WATER_SENSOR_PIN, VWPQ_COOLANT_SHORTAGE_PIN, VWPQ_OIL_PRESSURE_SWITCH_PIN, VWPQ_HANDBRAKE_INDICATOR_PIN, VWPQ_BRAKE_FLUID_WARNING_PIN);
  ClusterConfiguration defaultClusterConfig = cluster.clusterConfig();
#elif CLUSTER == 99
  // Golf 7 Passthrough mode
  MCP_CAN CAN2(VWMQB_PASS_CAN2_CS);  // Set CS pin

  long unsigned int canRxId2;
  unsigned char canRxLen2 = 0;
  unsigned char canRxBuf2[8];

  #include "src/Clusters/VW_MQB/VWMQBCluster.h"
  VWMQBCluster cluster(CAN2, ANALOG_FUEL_POT_INC, ANALOG_FUEL_POT_DIR, ANALOG_FUEL_POT_CS1, ANALOG_FUEL_POT_CS2, true);
  ClusterConfiguration defaultClusterConfig = cluster.clusterConfig();
#endif

// Game simulation variables
ClusterConfiguration clusterConfig = ClusterConfiguration::updatedFromDefaults(defaultClusterConfig, SPEED_CORRECTION_FACTOR, RPM_CORRECTION_FACTOR, MAXIMUM_RPM, MAXIMUM_SPEED, MINIMUM_COOLANT_TEMPERATURE, MAXIMUM_COOLANT_TEMPERATURE, ANALOG_FUEL_POT_MINIMUM_VALUE, ANALOG_FUEL_POT_MAXIMUM_VALUE, ANALOG_FUEL_POT_MINIMUM_VALUE2, ANALOG_FUEL_POT_MAXIMUM_VALUE2);
GameState game(clusterConfig);
SimhubGame simhubGame(game);

#if WIFI_ENABLED == 1
  // Wifi/web portal variables
  #include "src/Other/WifiFunctions.h"
  #include "src/Other/WebDashboard.h"

  #include "src/Games/ForzaHorizonGame.h"
  #include "src/Games/BeamNGGame.h"

  WifiFunctions wifiFunctions;
  WebDashboard webDashboard(game, WIFI_WEB_DASHBOARD_PORT, WIFI_WEB_DASHBOARD_UPDATE_INTERVAL);

  ForzaHorizonGame forzaHorizonGame(game, WIFI_FORZA_UDP_PORT);
  BeamNGGame beamNGGame(game, WIFI_BEAM_UDP_PORT);
#endif 

// Serial JSON parsing
JsonDocument doc;

void setup() {
  // Define the outputs
  pinMode(SPI_CS_PIN, OUTPUT);
  pinMode(CAN_INT, INPUT);

  //Begin with Serial Connection
  Serial.begin(SERIAL_BAUD_RATE);

  #if WIFI_ENABLED == 1
    wifiFunctions.begin(WIFI_CONFIG_PORTAL_ACCESS_POINT_NAME, WIFI_CONFIG_PORTAL_ACCESS_POINT_PASSWORD, WIFI_CONFIG_PORTAL_TIMEOUT);
    webDashboard.begin();
    forzaHorizonGame.begin();
    beamNGGame.begin();
  #endif

  simhubGame.begin();

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

  #if CLUSTER == 99
    pinMode(VWMQB_PASS_CAN2_INT, INPUT);

    //Begin with CAN Bus 2 Initialization
START_INIT2:
    if (CAN_OK == CAN2.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ))  // init can bus : baudrate = 500k
    {
      Serial.println("CAN2 BUS Shield init ok!");
    } else {
      Serial.println("CAN2 BUS Shield init fail");
      Serial.println("Init CAN2 BUS Shield again");
      delay(100);
      goto START_INIT2;
    }
    CAN2.setMode(MCP_NORMAL);
  #endif
}

void loop() {
  // Update the cluster with current state of the game
  cluster.updateWithGame(game);

  // Serial message handling
  readSerialJson();

  // Handle data from connected CAN hardware
  readCanBuffer();

  // Update the web dashboard
  #if WIFI_ENABLED == 1
    webDashboard.update();
  #endif
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

      //Serial.print("I got: @");
      //Serial.print(message);
      //Serial.println("@");

      DeserializationError error = deserializeJson(doc, message);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        message_pos = 0;
        return;
      }

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

        unsigned char DataToSend[8] = { p1, p2, p3, p4, p5, p6, p7, p8 };
        CAN.sendMsgBuf(address, 0, 8, DataToSend);
      } else if (action == 10) {
        // Used to decode custom protocol from Simhub in the following format:
        // {"action":10, "spe":54, "gea":"2", "rpm":3590, "mrp":7999, "lft":0, "rit":0, "oit":0, "pau":0, "run":0, "fue":0, "hnb":0, "abs":0, "tra":0}
        simhubGame.decodeSerialData(doc);
      }

      //Reset for the next message
      message_pos = 0;
    }
  }
}

void readCanBuffer() {
  if (!digitalRead(CAN_INT)) {                      // If CAN0_INT pin is low, read receive buffer
    CAN.readMsgBuf(&canRxId, &canRxLen, canRxBuf);  // Read data: len = data length, buf = data byte(s)

    #if CLUSTER == 99
      cluster.handleReceivedData(canRxId, canRxLen, canRxBuf);
    #endif

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

  #if CLUSTER == 99
    // From cluster to car
    if (!digitalRead(VWMQB_PASS_CAN2_INT)) {                      // If CAN0_INT pin is low, read receive buffer
      CAN2.readMsgBuf(&canRxId2, &canRxLen2, canRxBuf2);  // Read data: len = data length, buf = data byte(s)

      // Forward anything that we get back to the car
      CAN.sendMsgBuf(canRxId2, canRxLen2, canRxBuf2);
    }
  #endif
}
