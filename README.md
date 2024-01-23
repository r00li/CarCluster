# CarCluster

**Control car instrument clusters from your computer for gaming or other purposes using an ESP32.**

![Main image](https://github.com/r00li/CarCluster/blob/main/Misc/main_display.jpg?raw=true)

## Tell me more?

Ever wondered if you could use a car instrument cluster outside of your car? Well... the answer to that question is yes. The aim of this project is to give you the ability to run a real instrument cluster from a real car on your desk. The main purpose of this is gaming, but the possibilities are endless. You might want to use it as a clock. Or maybe you want to show the amount of unread emails - possibilities are endless.

Here are a few demo videos of various instrument clusters in action:

| [![](https://github.com/r00li/CarCluster/blob/main/Misc/thumb_vw_pq.png?raw=true )](https://youtu.be/-aPPZKZ644M) | [![](https://github.com/r00li/CarCluster/blob/main/Misc/thumb_vw_mqb.png?raw=true )](https://youtu.be/H56cSgZeaOw ) | [![](https://github.com/r00li/CarCluster/blob/main/Misc/thumb_bmwf.png?raw=true)](https://youtu.be/-aPPZKZ644M) |
|--|--|--|
| VW PQ | VW MQB | BMW F |

## What do I need?
The hardware for this is pretty straightforward. You will need:

 - ESP32 board (I am using a generic Devkit V1 board - available from ebay for around 10€)
 - MCP2515 CAN bus module (I am using a generic one from ebay - available for around 5€)
 - 12V power supply (doesn't have to be too powerful. But 1A or more is probably what you want)
 - Car instrument cluster supported by this project
 - Some wires, pin headers and other stuff to wire everything together
 - An X9C102 (or two) digital potentiometer plus two 1k ohm resistors (for fuel level simulation - optional)

### Supported instrument clusters
Currently fully tested and supported are the following instrument clusters:

- VW Polo 5 (Also known as Polo 6R, PQ25 platform)
- Škoda Superb 2 (PQ46 platform)
- VW T-Cross* (MQB platform)
- VW Golf 7 (MQB platform)
- BMW 5 Series (F10, BMW F platform)
- Mini Cooper (third generation, F55/F56/F57, BMW F platform)

This project supports both the older VW PQ platform cars (like the Polo and Superb) and newer VW MQB platform cars. Your best bet is probably the Polo cluster since it has the most features tested and working. MQB clusters are still missing some functionality (mostly various indicators). For both platforms some modifications might be needed based on the specific car model, so if you are unsure, get the specific clusters mentioned here.

***Newer MQB platform clusters like the T-Cross should be avoided**. After some (unknown) time they will enter into component protection mode which will flash the display and make most of the indicators useless. There is no known solution for this as of yet. I highly recommend you stick to older cars like Golf 7.

You can get these clusters from ebay, your local scrapyard, or if you are in EU: [rrr.lt](rrr.lt/) . No matter where you get them, they should cost you around 50€.

### Supported games
If you want to use the instrument cluster for gaming then currently supported are the following games:

- Forza Horizon 4 (Horizon 5 should also work, as should Forza Motorsport 7)
- BeamNG.drive
- Simhub (not a game but a tool to interface with other games)
  
## Set it up
### Wiring for VW PQ platform cluster
Below is the connector pinout for the Polo 6R dashboard:
![Pinout indication](https://github.com/r00li/CarCluster/blob/main/Misc/pinout.jpg?raw=true)You can also see my improvised connector. Just two rows of standard 2.54mm female pin headers glued together. If you can I would suggest you get a cluster that comes with it's original connector.

Wire everything according to the tables below:

| Cluster pin | Connect to | Comment |
|--|--|--|
| 31, 32 | +12V |
| 16 | GND (12V power supply) |
| 20 | GND (ESP) |
| 28 | CAN H (connect to your can bus interface) |
| 29 | CAN L (connect to your can bus interface) |
| 27 | ESP pin D15 | Oil pressure switch |
| 17 | ESP pin D4 | Sprinkler water level sensor |
| 18 | ESP pin D16 | Coolant shortage indicator |
| 1 | Digital Pot FS1 | Fuel level indicator pin 1 |
| 2 | Digital Pot FS2 | Fuel level indicator pin 2 |
| 3 | Digital Pot FS3 | Fuel level indicator 2 pin 1 (some clusters) |
| 4 | Digital Pot FS4 | Fuel level indicator 2 pin 2  (some clusters) |
| 25 | ESP pin D13 | Handbrake indicator |
| 26 | ESP pin D22 | Brake fluid warning (PQ46 only) |
| 21 | Through a push button to ESP GND | Optional: Trip computer menu up |
| 22 | Through a push button to ESP GND | Optional: Trip computer menu set/reset |
| 23 | Through a push button to ESP GND | Optional: Trip computer menu down |
| 19 | Through a NTC (10k?) | Optional: outside temperature sensor |

### Wiring for VW MQB platform cluster
Below is the connector pinout for the VW T-Cross dashboard:
![Pinout indication](https://github.com/r00li/CarCluster/blob/main/Misc/pinout_mqb.jpg?raw=true)

| Cluster pin | Connect to | Comment |
|--|--|--|
| 1 | +12V |
| 10 | GND (12V power supply) |
| 18 | CAN H (connect to your can bus interface) |
| 17 | CAN L (connect to your can bus interface) |
| 11 | ESP/Digital Pot GND | Fuel level indicator GND |
| 14 | Digital Pot FS1 | Fuel level indicator pin 1 |
| 25 | Digital Pot FS2 | Fuel level indicator pin 2 |

### Wiring for BMW F series cluster

| Cluster pin | Connect to | Comment |
|--|--|--|
| 1, 2 | +12V |
| 4, 5 | Through a NTC (10k?) | Optional: outside temperature sensor |
| 7, 8 | GND (12V power supply) |
| 11 | +12V | Wakeup signal |
| 6 | CAN H (connect to your can bus interface) |
| 12 | CAN L (connect to your can bus interface) |

### Other wiring

Connect the CAN bus interface to ESP32 according to this:
| Can bus interface pin | Connect to | Comment |
|--|--|--|
| INT | ESP Pin D2 |
| CS | ESP Pin D5 |
| SCK | ESP Pin D18 |
| MISO | ESP Pin D19 |
| MOSI | ESP Pin 23 |
| GND | ESP GND |
| VCC | ESP VIN | This is the 5V line when ESP is powered from USB |

### Fuel level simulation
Most instrument clusters use analog resistance values in order to calculate fuel level. The best way to simulate that is through the use of an X9C102 digital potentiometer. Connect it according to the schematic below. You will need two 1k ohm resistors in addition to the digital potentiometer itself. 

When connecting the digital pot you might find that if you increase the value in the dashboard, the value on the cluster goes down. If this is the case swap connections FS1 and FS2. 

If your cluster needs two fuel level senders then duplicate the below schematic (two digital pots). Replace FS1 and FS2 connections with FS3 and FS4, connect the CS pin of the second digital pot to ESP pin D33. Keep other connections the same.

![Fuel level simulation](https://github.com/r00li/CarCluster/blob/main/Misc/fuel_simulation.png?raw=true)

### Install the arduino sketch to the ESP32
Navigate to the Dashboard Sketches folder of this project and select the sketch for the dashboard that you are using. Download the sketch folder and open it using Arduino IDE (I am using version 2.1.0).

If you haven't yet installed ESP32 support for Arduino IDE then do so now. Go to Boards Manager and search for esp32 by Espressif Systems. I am using version 2.0.7.

Finally you will need to install a few libraries:

 - [mcp_can by coryjfowler](https://github.com/coryjfowler/MCP_CAN_lib) - install through library manager - tested using 1.5.0
 - [ArduinoJson](https://arduinojson.org/) - install through library manager - tested using 6.20.1
 - [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) - install manually by downloading from github and placing it into your arduino IDE libraries folder
 - [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) - install manually by downloading from github and placing it into your arduino IDE libraries folder
 - [ESPDash](https://docs.espdash.pro/) - install through library manager - tested using 4.0.1
 - [X9C10X](https://github.com/RobTillaart/X9C10X) - install through library manager - tested using 0.2.2
 - [WifiManager](https://github.com/tzapu/WiFiManager) - install through library manager - tested using 2.0.16-rc.2
 - [MultiMap](https://github.com/RobTillaart/MultiMap) - Install through library manager - tested using 0.2.0

Now that you have done that you can compile and install the sketch to your ESP32. Upon starting the ESP will create a wifi network access point called `CarCluster`. Connect to it using your phone/laptop using the password `carcluster`. After you have done that you can open a web browser (if a popup one doesn't open automatically upon connection) and navigate to `192.168.4.1`. This will bring up WifiManager where you can see the networks around you and connect to the one you want. After you do that your ESP will automatically connect to the network you select unless an error occurs in which case the access point will be created again. If you do not configure the wifi network in 3 minutes the ESP will boot in serial only mode that you can use with Simhub. 

*Note that you might have some trouble uploading the code to the arduino while the CAN interface is connected. If you do, disconnect the CAN interface and try uploading again.*  

### Use the web interface to test the basic functionality

![Web dashboard](https://github.com/r00li/CarCluster/blob/main/Misc/screenshot_dashboard.png?raw=true)
This project contains a small web server that you can use to control some basic functionality of the dashboard. You can set the RPM, speed and some of the other parameters using it. Simply open `http://<ip-of-your-ESP32>/` in your web browser and you should be able to see the above interface. Move the sliders and click the buttons to control the dashboard display.

### Use telemetry data from your game
*For forza Horizon 4/5*:
 1.  Launch the game and proceed through the menus until you can drive your car
 2.  Pause the game and navigate to the Settings menu
 3.  Navigate to HUD and Gameplay
 4.  Set `Data Out` to `ON`
 5.  Set `Data Out IP Address` to `ip-of-your-ESP32`
 6.  Set `Data Out IP Port` to `1101` (or other if you changed the IP in the code)

*For BeamNG.drive:*

 1. Navigate to options
 2. Select `Others`
 3. Scroll down until you find `OutGauge support` and enable it
 4. Set IP address to `ip-of-your-ESP32`
 5. Set port to `1102` (or other if you changed the IP in the code)

Whenever you now run the game you should see the instrument cluster showing the data from the game. 

### Use with Simhub
Simhub support is also included. Simhub can be used in cases where your favourite game isn't supported by this project directly. Simhub support is for now only available through USB connection and not through wifi. 

In order to set this up you will first need to [enable custom serial devices plugin](https://github.com/SHWotever/SimHub/wiki/Custom-serial-devices#enabling-the-plugin) in simhub. After you have done that navigate to the "Custom serial devices" tab on the left and add a new device. Configure it as follows:

![Simhub configuration](https://github.com/r00li/CarCluster/blob/main/Misc/simhub.png?raw=true)

In the Update messages enter the following code:

     '{"action":10' +
     ', "spe":' + truncate(format([SpeedKmh], 0)) + 
     ', "gea":"' + isnull([Gear], 'P') + '"' +
     ', "rpm":' + truncate(isnull([Rpms], 0)) +
     ', "mrp":' + truncate(isnull([MaxRpm], 8000)) +
     ', "lft":' + isnull([TurnIndicatorLeft], 0) +
     ', "rit":' + isnull([TurnIndicatorRight], 0) +
     ', "oit":' + truncate(isnull([OilTemperature], 0)) + 
     ', "pau":' + isnull([DataCorePlugin.GamePaused], 0) +
     ', "run":' + isnull([DataCorePlugin.GameRunning], 0) +
     ', "fue":' + truncate(isnull([DataCorePlugin.Computed.Fuel_Percent], 0)) +
     ', "hnb":' + isnull([Handbrake], 0) +
     ', "abs":' + isnull([ABSActive], 0) +
     ', "tra":' + isnull([TCActive], 0) +
     '}\n'

You can now enjoy CarCluster with any game that is supported by Simhub.

### Known issues and limitations
For the most part everything should work without issues. However there are a few small things still being worked on:

- (PQ46 only) Multifunction menu doesn't work.
- (PQ46 only) Some warnings will show on start and a steering wheel assist light will be lit up.
- (MQB only) less important features like ABS/ESP indicators, some menu controls, ... do not work yet.
- (MQB only) some warning messages regarding assist systems will show up on the display upon first start. Depends on your particular cluster

## Help and support

If you need help with anything contact me through github (open an issue here). You can also check my website for other contact information: http://www.r00li.com .

If you want me to add support for a game or a specific instrument cluster you can also [donate to this project using Paypal](https://www.paypal.com/donate/?hosted_button_id=NVTTFT7Z6KG3E). This will not guarantee that I will add support, but it will certainly give me more incentive to do so.

## Acknowledgements
This project would not be possible without various people sharing their knowledge and results of their experiments. In particular I would like to thank the following individuals:

- [Leon Bataille](https://hackaday.io/Lebata30) for posting his initial Polo cluster work on Hackaday. The Polo instrument cluster sketch for this project is based on his work
- [Ronaldo Cordeiro](https://github.com/ronaldocordeiro) for providing information (including sample code) for MQB cluster integration
- [Ross-tech forum user jyoung8607](https://forums.ross-tech.com/index.php?members/208/) for providing CAN scans of various cars used in this project

## License

This work is licensed under GNU GPL version 3. See LICENSE file for more details
