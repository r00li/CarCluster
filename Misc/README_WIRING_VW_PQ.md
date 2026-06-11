### Wiring for VW PQ platform cluster
First begin by wiring your CAN interface/ESP32 board - see the main readme for details. Then follow the below instructions:

Below is the connector pinout for the Polo 6R instrument cluster:
![Pinout indication](https://github.com/r00li/CarCluster/blob/main/Misc/pinout.jpg?raw=true)
You can also see my improvised connector. Just two rows of standard 2.54mm female pin headers glued together. If you can I would suggest you get a cluster that comes with it's original connector.

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