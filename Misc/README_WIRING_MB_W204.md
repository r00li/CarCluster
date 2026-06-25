### Wiring for Mercedes Benz C-Class W204 cluster
First begin by wiring your CAN interface/ESP32 board - see the main readme for details. Then follow the below instructions:

Below is the connector pinout for the Mercedes W204 instrument cluster:
![Pinout indication](https://github.com/r00li/CarCluster/blob/main/Misc/pinout_w204.jpg?raw=true)
You can also see my improvised connector. Just two rows of standard 2.54mm female pin headers glued together. If you can I would suggest you get a cluster that comes with it's original connector.

Note that this is a dual CAN cluster, so you will need a second CAN interface board. Wire the second CAN interface using the same SPI pins, and connect CS of the second interface to pin D25 on the ESP, and INT pin of the interface to pin D27 on the ESP. 

Enable the internal termination resistors for both of the CAN interfaces. 

Wire the cluster according to the table below:

| Cluster pin | Connect to | Comment |
|--|--|--|
| 1 | GND (12V power supply) |
| 4 | 12V |
| 12 | CAN L1 (connect to your can bus interface) | CAN E bus |
| 13 | CAN H1 (connect to your can bus interface) | CAN E bus |
| 17 | CAN L2 (connect to your can bus interface) | CAN B bus |
| 18 | CAN H2 (connect to your can bus interface) | CAN B bus |
