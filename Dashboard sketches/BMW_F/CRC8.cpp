// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// 
// ####################################################################################################################

#include "CRC8.h"

CRC8::CRC8(void) {
}

void CRC8::begin(void) {
  crc  remainder;
    for (int dividend = 0; dividend < 256; ++dividend)
    {
        remainder = dividend << (WIDTH - 8);

        for (uint8_t bit = 8; bit > 0; --bit)
        {  
            if (remainder & TOPBIT)
            {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }
        crcTable[dividend] = remainder;
    }
}

crc CRC8::get_crc8(uint8_t const message[], int nBytes, uint8_t final) {
  uint8_t data;
  crc remainder = 0xFF;

  for (int byte = 0; byte < nBytes; ++byte) {
      data = message[byte] ^ (remainder >> (WIDTH - 8));
      remainder = crcTable[data] ^ (remainder << 8);
  }
	remainder = remainder^final;
   
  return (remainder);
}
