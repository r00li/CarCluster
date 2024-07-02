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
// 1 = BMW F10 (BMW F series)
// 2 = Mini F55 (BMW F series)
// 3 = Golf 7 (VW MQB)
// 4 = Polo 6R (VW PQ25)
// 5 = Skoda Superb 2 (VW PQ46)
#define CLUSTER 2

// Configure the maximum RPM value shown on the cluster
#define MAXIMUM_RPM 7000

// A correction factor for the RPM value. RPM will be multiplied by this value.
// This enables you to fix displayed values that are slightly off, though you might not be able
// to ever fully get them calibrated correctly - depending on the cluster. 
#define RPM_CORRECTION_FACTOR 1.0

// Configure the maximum speed shown on the cluster (in km/h)
#define MAXIMUM_SPEED 255

// A correction factor for the speed. Speed will be multiplied by this value.
// This enables you to fix displayed values that are slightly off, though you might not be able
// to ever fully get them calibrated correctly - depending on the cluster. 
#define SPEED_CORRECTION_FACTOR 1.0

// Define the minimum and maximum coolant temperature your cluster can display
// Leave alone if your cluster has no such display
#define MINIMUM_COOLANT_TEMPERATURE 50
#define MAXIMUM_COOLANT_TEMPERATURE 150

// Select if you want Wifi to be enabled or not.
// Wifi gives you a web dashboard that you can use for testing
// and the ability to connect to certain games directly, but will only work on an ESP32. 
// If using a different board select 0.
//
// In order to connect to wifi the ESP will on first boot create a wifi access point called CarCluster. Connect to it (password is "carcluster"),
// then open your web browser and navigate to 192.168.4.1 and use the UI there to connect your wifi network.
// If wifi is not connected after 3 minutes the ESP will continue normal operation and you can use it in Simgub/serial mode
// 
// 1 for enabled
// 0 for disabled
#define WIFI_ENABLED 1

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
#define SERIAL_BAUD_RATE 921000

// UDP/TCP ports used for various games and other stuff.
// Only applicable if wifi is enabled.
#define WIFI_FORZA_UDP_PORT 1101
#define WIFI_BEAM_UDP_PORT 1102
#define WIFI_WEB_DASHBOARD_PORT 80

// ------------------------ END OTHER CONFIGURATION -------------------


// Libraries
#include <mcp_can.h>  // CAN Bus Shield Compatibility Library (install through library manager: mcp_can by coryjfowler - tested using 1.5.0)
#include <SPI.h>      // CAN Bus Shield SPI Pin Library (arduino system library)
#include <AsyncUDP.h>     // For game integration (system library part of ESP core)
#include <ArduinoJson.h>  // For parsing serial data and for ESPDash (install through library manager: ArduinoJson by Benoit Blanchon - tested using 7.0.4)
#include "X9C10X.h"       // For fuel level simulation (install through library manager: X9C10X by Rob Tillaart - tested using 0.2.2)
#include "MultiMap.h"     // For fuel level calculation - supports non linear mapping found on BMW clusters (install through library manager)

#include "WiFiManager.h"        // For easier wifi management (install through library manager: WiFiManager by tzapu - tested using 2.0.16-rc.2)
#include <WiFi.h>               // Arduino system library (part of ESP core)
#include <AsyncTCP.h>           // Requirement for ESP-DASH (install manually from: https://github.com/me-no-dev/AsyncTCP )
#include <ESPAsyncWebServer.h>  // Requirement for ESP-DASH (install manually from:  https://github.com/me-no-dev/ESPAsyncWebServer )
#include <ESPDash.h>            // Web dashboard ( install from library manager: ESP-DASH by Ayush Sharma - tested using 4.0.1)

#include "src/Games/GameSimulation.h"
#include "src/Games/ForzaHorizonGame.h"
#include "src/Games/BeamNGGame.h"
#include "src/Games/SimhubGame.h"
#include "src/Other/WifiFunctions.h"
#include "src/Other/WebDashboard.h"

// CAN bus configuration
MCP_CAN CAN(SPI_CS_PIN);  // Set CS pin

// CAN bus Receiving
long unsigned int canRxId;
unsigned char canRxLen = 0;
unsigned char canRxBuf[8];
char canRxMsgString[128];  // Array to store serial string

// Cluster initialization
#if CLUSTER == 1
#include "src/Clusters/BMW_F/BMWFSeriesCluster.h"
BMWFSeriesCluster cluster(CAN, false);
ClusterConfiguration defaultClusterConfig = cluster.clusterConfig();
#elif CLUSTER == 2
#include "src/Clusters/BMW_F/BMWFSeriesCluster.h"
BMWFSeriesCluster cluster(CAN, true);
ClusterConfiguration defaultClusterConfig = cluster.clusterConfig();
#endif

// Game simulation variables
ClusterConfiguration clusterConfig = ClusterConfiguration::updatedFromDefaults(defaultClusterConfig, SPEED_CORRECTION_FACTOR, RPM_CORRECTION_FACTOR, MAXIMUM_RPM, MAXIMUM_SPEED, MINIMUM_COOLANT_TEMPERATURE, MAXIMUM_COOLANT_TEMPERATURE);
GameState game(clusterConfig);
ForzaHorizonGame forzaHorizonGame(game, WIFI_FORZA_UDP_PORT);
BeamNGGame beamNGGame(game, WIFI_BEAM_UDP_PORT);
SimhubGame simhubGame(game);

// Wifi/web portal variables
WifiFunctions wifiFunctions;
WebDashboard webDashboard(game, WIFI_WEB_DASHBOARD_PORT, WIFI_WEB_DASHBOARD_UPDATE_INTERVAL);

// Serial JSON parsing
JsonDocument doc;

void setup() {
  // Define the outputs
  pinMode(SPI_CS_PIN, OUTPUT);
  pinMode(CAN_INT, INPUT);

  //Begin with Serial Connection
  Serial.begin(SERIAL_BAUD_RATE);

  wifiFunctions.begin(WIFI_CONFIG_PORTAL_ACCESS_POINT_NAME, WIFI_CONFIG_PORTAL_ACCESS_POINT_PASSWORD, WIFI_CONFIG_PORTAL_TIMEOUT);
  webDashboard.begin();
  forzaHorizonGame.begin();
  beamNGGame.begin();

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
}

void loop() {
  // Update the cluster with current state of the game
  cluster.updateWithGame(game);

  // Serial message handling
  readSerialJson();

  // Handle data from connected CAN hardware
  readCanBuffer();

  // Update the web dashboard
  webDashboard.update();
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
