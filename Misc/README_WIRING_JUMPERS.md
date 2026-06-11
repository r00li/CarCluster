# Readme for wiring with jumpers/breadboard

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

### Fuel level simulation for clusters with analog fuel level indicators
Most instrument clusters use analog resistance values in order to calculate fuel level. The best way to simulate that is through the use of an X9C102 digital potentiometer. Connect it according to the schematic below. You will need two 1k ohm resistors in addition to the digital potentiometer itself. 

When connecting the digital pot you might find that if you increase the value in the dashboard, the value on the cluster goes down. If this is the case swap connections FS1 and FS2. 

If your cluster needs two fuel level senders then duplicate the below schematic (two digital pots). Replace FS1 and FS2 connections with FS3 and FS4, connect the CS pin of the second digital pot to ESP pin D33. Keep other connections the same.

![Fuel level simulation](https://github.com/r00li/CarCluster/blob/main/Misc/fuel_simulation.png?raw=true)
