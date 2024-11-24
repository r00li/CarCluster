# CarCluster PCB

Intended to make the wiring of instrument clusters easier. But can just as well be easily used for any other purpose where CAN interface (or two) is needed. Requires just the CAN breakout, ESP32, some screw terminals and pin headers to be built.

Additionally it includes the ability to simulate fuel level on certain instrument clusters - see the main page for details.

![Finished PCB](https://github.com/r00li/CarCluster/blob/main/PCB/pcb_finished.jpg?raw=true)

## Where to get the PCB?
You have two easy solutions... if you will be using JLCPCB for manufacturing you can head over to [OSHW lab project page](https://oshwlab.com/r00li/carcluster-pcb). There you can order PCBs directly from the EasyEDA web editor by simply clicking "Open in editor" under the PCB and then going to "Fabrication" > "One Click order PCB/SMT". The PCBs will probably cost you around 30€ or less for manufacturing and shipping to your address.

Alternatively you can download the .zip file with gerbers from this project and then uploading them to your favourite PCB manufacturers website. 

If you want to make any changes, you can again use the OSHW lab website directly, or download the project files and open them in EasyEDA (standard edition). 

## BOM:
- PCB
- Screw terminals (10x2pin or 5x4pin, I use generic blue ones)
- Female 2.54mm pin headers (two rows should be enough)
- ESP 32 Devkit v1 (other ESP32 modules with same pin layout should work)
- MCP2515 CAN breakout board (1x or 2x if you require dual CAN)
- DC power input jack (for 12V power to the cluster), alternatively you can also use another 2 pin screw terminal (depending on your needs)
- X9C102 digital potentiometer (2x, optional - for fuel level simulation)
- 1k resistor (4x, optional - for fuel level simulation)
- 2 pin 2.54mm pin male header and jumper or a small piece of wire (optional - for connecting 12V power supply ground to ESP32 ground)

## Assembly notes:
All of the components should be placed on the top of the board (see picture for more details). Place the components by height - tallest components should be placed last.
 
Female headers should be used for all both ESP32 and CAN interfaces. That way you can always remove them if needed (ESP32 usually needs to be removed while programming). ESP32 is placed right side up, while CAN interfaces are mounted inverted (see picture).
 
Small tip for cutting header pins - Use wire cutters. Count how many pins you need, then place wire cutters one pin after the number of pins you have counted and cut. This will ensure that you get the correct amount of pins. Sand or cut off the excess plastic from the edges to make a nicer looking header.

## Layout and schematic preview

Main layout:
![PCB layout](https://github.com/r00li/CarCluster/blob/main/PCB/pcb_layout.png?raw=true)

Schematic:
![PCB schematic](https://github.com/r00li/CarCluster/blob/main/PCB/pcb_schematic.png?raw=true)
