### Wiring for VW MQB platform cluster
First begin by wiring your CAN interface/ESP32 board - see the main readme for details. Then follow the below instructions:

Below is the connector pinout for the VW T-Cross instrument cluster:
![Pinout indication](https://github.com/r00li/CarCluster/blob/main/Misc/pinout_mqb.jpg?raw=true)

| Cluster pin | Connect to | Comment |
|--|--|--|
| 1 | +12V |
| 10 | GND (12V power supply) |
| 18 | CAN H (connect to your can bus interface) |
| 17 | CAN L (connect to your can bus interface) |
| 11 | ESP/Digital Pot GND | Fuel level indicator GND |
| 14 | Digital Pot FS1 | Fuel level indicator pin 1 |
| 15 | Digital Pot FS2 | Fuel level indicator pin 2 |

VW Golf uses an almost identical pinout, just with a few additional pins for fuel level:

| Cluster pin | Connect to | Comment |
|--|--|--|
| 1 | +12V |
| 10 | GND (12V power supply) |
| 18 | CAN H (connect to your can bus interface) |
| 17 | CAN L (connect to your can bus interface) |
| 11 | ESP/Digital Pot GND | Fuel level indicator GND |
| 14 | Digital Pot FS1 | Fuel level indicator pin 1 |
| 15 | Digital Pot FS2 | Fuel level indicator pin 2 |
| 12 | Digital Pot FS3 | Fuel level indicator 2 pin 1 |
| 13 | Digital Pot FS4 | Fuel level indicator 2 pin 2 |