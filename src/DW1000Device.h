#pragma once

#define INACTIVITY_TIME 2000

#include "DW1000Time.h"
#include "portable.h"

class DW1000Device
{
public:
	// Constructor and destructor
	DW1000Device(PortableCode &);
	DW1000Device(PortableCode &, uint8_t shortAddress[]);
	void operator=(const DW1000Device &dev) { _portable = dev._portable; }

	~DW1000Device();

	// Setters
	void setShortAddress(uint8_t address[]);
	void setPayload(float payload) { _payload = payload; }
	void setIndex(uint8_t index) { _index = index; }
	void setRange(float range) { _range = range; }
	void setRXPower(float power) { _RXPower = power; }
	void setFPPower(float power) { _FPPower = power; }
	void setQuality(float quality) { _quality = quality; }
	void setReplyTime(uint16_t replyDelayTimeUs) { _replyDelayTimeUs = replyDelayTimeUs; }

	// Getters
	uint8_t getIndex() { return _index; }
	uint8_t *getByteShortAddress() { return _shortAddress; }
	uint16_t getShortAddress();
	uint16_t getReplyTime() { return _replyDelayTimeUs; }

	float getRange() { return _range; }
	float getPayload() { return _payload; }
	float getRXPower() { return _RXPower; }
	float getFPPower() { return _FPPower; }
	float getQuality() { return _quality; }

	bool isAddressEqual(DW1000Device *device);
	bool isShortAddressEqual(DW1000Device *device);

	// functions which contains the date: (easier to put as public)
	//  timestamps to remember
	DW1000Time timePollSent;
	DW1000Time timePollReceived;
	DW1000Time timePollAckSent;
	DW1000Time timePollAckReceived;
	DW1000Time timeRangeSent;
	DW1000Time timeRangeReceived;

	bool hasSentPollAck;
	bool hasRangeBeenServed;

	DW1000Time timePollAckReceivedMinusPollSent;
	DW1000Time timeRangeSentMinusPollAckReceived;

	void noteActivity();
	bool isInactive();
	void setExpectedMessageID(uint8_t id) { _expectedMessageID = id; }
	uint8_t getExpectedMessageID() { return _expectedMessageID; }

private:
	PortableCode &_portable;
	uint8_t _shortAddress[2];
	unsigned long _activity;
	uint16_t _replyDelayTimeUs;
	uint8_t _index;

	float _range;
	float _RXPower;
	float _FPPower;
	float _quality;
	float _payload;
	uint8_t _expectedMessageID;
};
