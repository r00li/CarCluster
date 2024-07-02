// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#ifndef CRC8_H
#define CRC8_H

#include "Arduino.h"

// CRC is calculated by using CRC8_SAE_J1850, but with a final XOR value specific to each message
// Use calculator here to determine the correct sequence: http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
// Polynomial 0x1D, initial value 0xFF, Final XOR value XXX.
// Use a known good message and vary the final XOR value until you get the desired result. This is your final XOR value that you can use.
// More information here: https://www.reddit.com/r/CarHacking/comments/tufn5t/bmw_can_message_crc_checksum/

typedef uint8_t crc;
#define POLYNOMIAL 0x1D
#define WIDTH (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))

class CRC8 {
	public:
	  CRC8();
	  void begin();
	  crc get_crc8(uint8_t const message[], int nBytes, uint8_t final);
	 
	private:
	  uint8_t crcTable[256];
};

#endif
