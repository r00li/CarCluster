### Wiring for BMW E60 cluster
First begin by wiring your CAN interface/ESP32 board - see the main readme for details. Then follow the below instructions:

Below is the connector pinout for the BMW E60 instrument cluster:
![Pinout indication](https://github.com/r00li/CarCluster/blob/main/Misc/pinout_bmw_e.jpg?raw=true)

| Cluster pin | Connect to | Comment |
|--|--|--|
| 9 | +12V |
| 4, 5 | Through a NTC | Optional: outside temperature sensor |
| 18 | GND (12V power supply) |
| 11, 12 | +12V | To get rid of washer fluid, coolant lights |
| 6 | CAN H (connect to your can bus interface) |
| 7 | CAN L (connect to your can bus interface) |
| 2, 15 | Fuel sim A | Pair A for fuel simulation. Wire a 220ohm R between.
| 3, 16 | Fuel sim B | Pair B for fuel simulation. Wire a 220ohm R between.
| 13 | ESP pin D13 | Parking brake simulation

Note that currently only static fuel is supported (using the above wiring). Wiring two 220 ohm resistors between each pair will give you about half a fuel tank. Increase the resistance to get more fuel. Proper simulation is work in progress. 

For parking brake simulation to work you need to wire ESP GND to 12V power supply GND.