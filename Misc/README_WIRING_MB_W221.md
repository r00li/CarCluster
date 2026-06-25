### Wiring for Mercedes Benz S-Class W221 cluster
First begin by wiring your CAN interface/ESP32 board - see the main readme for details. Then follow the below instructions:

Below is the connector pinout for the Mercedes W221 instrument cluster:
![Pinout indication](https://github.com/r00li/CarCluster/blob/main/Misc/pinout_w221.jpg?raw=true)
You can also see my improvised connector. Just two rows of standard 2.54mm female pin headers glued together. If you can I would suggest you get a cluster that comes with it's original connector.

Enable the internal termination resistors for your CAN interfaces. 

Wire the cluster according to the table below:

| Cluster pin | Connect to | Comment |
|--|--|--|
| 1 | GND (12V power supply) |
| 2 | GND (12V power supply) |
| 3 | GND (12V power supply) |
| 4 | 12V |
| 5 | 12V |
| 9 | GND (12V power supply) |
| 12 | CAN L (connect to your can bus interface) | CAN F bus |
| 13 | CAN H (connect to your can bus interface) | CAN F bus |
