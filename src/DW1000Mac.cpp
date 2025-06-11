#include "DW1000Mac.h"

// Constructor and destructor

DW1000Mac::DW1000Mac()
{
	_seqNumber = 0;
}

DW1000Mac::~DW1000Mac()
{
}

// for poll message we use just 2 bytes address
// total=4 bytes
void DW1000Mac::generateBlinkFrame(uint8_t frame[], uint8_t sourceShortAddress[])
{
	// Frame Control
	frame[0] = FC_1_BLINK;
	// sequence number
	frame[1] = _seqNumber;
	// source address (2 bytes)
	frame[2] = sourceShortAddress[1];
	frame[3] = sourceShortAddress[0];

	// we increment seqNumber
	incrementSeqNumber();
}

// the short fram usually for Resp, Final, or Report
// 2 bytes for Desination Address and 2 bytes for Source Address
// total=9 bytes
void DW1000Mac::generateShortMACFrame(uint8_t frame[], uint8_t sourceShortAddress[], uint8_t destinationShortAddress[])
{
	// Frame controle
	frame[0] = FC_1;
	frame[1] = FC_2_SHORT;
	// sequence number (11.3) modulo 256
	frame[2] = _seqNumber;
	// PAN ID
	frame[3] = 0xCA;
	frame[4] = 0xDE;

	// destination address (2 bytes)
	frame[5] = destinationShortAddress[1];
	frame[6] = destinationShortAddress[0];
	/*
	uint8_t destinationShortAddressReverse[2];
	reverseArray(destinationShortAddressReverse, destinationShortAddress, 2);
	memcpy(frame + 5, destinationShortAddressReverse, 2);
	*/

	// source address (2 bytes)
	frame[7] = sourceShortAddress[1];
	frame[8] = sourceShortAddress[0];
	/*
	uint8_t sourceShortAddressReverse[2];
	reverseArray(sourceShortAddressReverse, sourceShortAddress, 2);
	memcpy(frame + 7, sourceShortAddressReverse, 2);
	*/

	// we increment seqNumber
	incrementSeqNumber();
}

// the long frame for Ranging init
// 8 bytes for Destination Address and 2 bytes for Source Address
// total=15
void DW1000Mac::generateLongMACFrame(uint8_t frame[], uint8_t sourceShortAddress[], uint8_t destinationAddress[])
{
	// Frame controle
	frame[0] = FC_1;
	frame[1] = FC_2;
	// sequence number (11.3) modulo 256
	frame[2] = _seqNumber;
	// PAN ID
	frame[3] = 0xCA;
	frame[4] = 0xDE;

	// destination address (8 bytes) - we need to reverse the byte array
	frame[5] = destinationAddress[7];
	frame[6] = destinationAddress[6];
	frame[7] = destinationAddress[5];
	frame[8] = destinationAddress[4];
	frame[9] = destinationAddress[3];
	frame[10] = destinationAddress[2];
	frame[11] = destinationAddress[1];
	frame[12] = destinationAddress[0];
	/*
	uint8_t destinationAddressReverse[8];
	reverseArray(destinationAddressReverse, destinationAddress, 8);
	memcpy(frame + 5, destinationAddressReverse, 8);
	*/

	// source address (2 bytes)
	frame[13] = sourceShortAddress[1];
	frame[14] = sourceShortAddress[0];
	/*
	uint8_t sourceShortAddressReverse[2];
	reverseArray(sourceShortAddressReverse, sourceShortAddress, 2);
	memcpy(frame + 13, sourceShortAddressReverse, 2);
	*/

	// we increment seqNumber
	incrementSeqNumber();
}

void DW1000Mac::decodeBlinkFrame(uint8_t frame[], uint8_t shortAddress[])
{
	shortAddress[0] = frame[3];
	shortAddress[1] = frame[2];
}

void DW1000Mac::decodeShortMACFrame(uint8_t frame[], uint8_t address[])
{
	address[0] = frame[8];
	address[1] = frame[7];
	/*
	uint8_t reverseAddress[2];
	memcpy(reverseAddress, frame+7, 2);
	reverseArray(address, reverseAddress, 2);
	*/

	// we grab the destination address for the mac frame
	// uint8_t destinationAddress[2];
	// memcpy(destinationAddress, frame+5, 2);
}

void DW1000Mac::decodeLongMACFrame(uint8_t frame[], uint8_t address[])
{
	address[0] = frame[14];
	address[1] = frame[13];
	/*
	uint8_t reverseAddress[2];
	memcpy(reverseAddress, frame+13, 2);
	reverseArray(address, reverseAddress, 2);
	*/

	// we grab the destination address for the mac frame
	// uint8_t destinationAddress[8];
	// memcpy(destinationAddress, frame+5, 8);
}

inline void DW1000Mac::incrementSeqNumber()
{
	// normally overflow of uint8 automatically resets to 0 if over 255
	// but if-clause seems safer way
	if (_seqNumber == 255)
		_seqNumber = 0;
	else
		_seqNumber++;
}

inline void DW1000Mac::reverseArray(uint8_t to[], uint8_t from[], int16_t size)
{
	for (int16_t i = 0; i < size; i++)
	{
		*(to + i) = *(from + size - i - 1);
	}
}
