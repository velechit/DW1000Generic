#ifndef __DW1000MAC_H__
#define __DW1000MAC_H__

#define FC_1 0x41
#define FC_1_BLINK 0xC5
#define FC_2 0x8C
#define FC_2_SHORT 0x88

#define PAN_ID_1 0xCA
#define PAN_ID_2 0xDE

#define BLINK_MAC_LEN 4
#define SHORT_MAC_LEN 9
#define LONG_MAC_LEN 15


#include "DW1000Device.h" 

class DW1000Mac {
public:
	//Constructor and destructor
	DW1000Mac();
	~DW1000Mac();	
	
	//for poll message we use just 2 bytes address
	//total=12 bytes
	void generateBlinkFrame(uint8_t frame[], uint8_t sourceShortAddress[]);
	
	//the short fram usually for Resp, Final, or Report
	//2 bytes for Desination Address and 2 bytes for Source Address
	//total=9 bytes
	void generateShortMACFrame(uint8_t frame[], uint8_t sourceShortAddress[], uint8_t destinationShortAddress[]);
	
	//the long frame for Ranging init
	//8 bytes for Destination Address and 2 bytes for Source Address
	//total of
	void generateLongMACFrame(uint8_t frame[], uint8_t sourceShortAddress[], uint8_t destinationAddress[]);
	
	//in order to decode the frame and save source Address!
	void decodeBlinkFrame(uint8_t frame[], uint8_t shortAddress[]);
	void decodeShortMACFrame(uint8_t frame[], uint8_t address[]);
	void decodeLongMACFrame(uint8_t frame[], uint8_t address[]);
	


private:
	uint8_t _seqNumber = 0;
	inline void reverseArray(uint8_t to[], uint8_t from[], int16_t size);
	inline void incrementSeqNumber();
	
};


#endif /*! __DW1000MAC_H__ */

