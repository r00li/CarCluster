### Wiring for BMW F series cluster
First begin by wiring your CAN interface/ESP32 board - see the main readme for details. Then follow the below instructions:

Below is the connector pinout for the BMW F10 instrument cluster (same pinout is used for other F series cluster, though some clusters might not have the MOST bus pins):
![Pinout indication](https://github.com/r00li/CarCluster/blob/main/Misc/pinout_bmw_f.jpg?raw=true)

| Cluster pin | Connect to | Comment |
|--|--|--|
| 1, 2 | +12V |
| 4, 5 | Through a NTC (10k?) | Optional: outside temperature sensor |
| 7, 8 | GND (12V power supply) |
| 11 | +12V | Wakeup signal |
| 6 | CAN H (connect to your can bus interface) |
| 12 | CAN L (connect to your can bus interface) |
