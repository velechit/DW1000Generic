#pragma once

#include "DW1000.h"
#include "DW1000Time.h"
#include "DW1000Device.h"
#include "DW1000Mac.h"

// messages used in the ranging protocol
enum class MessageType : uint8_t
{
	POLL = 0,
	POLL_ACK = 1,
	RANGE = 2,
	RANGE_REPORT = 3,
	BLINK = 4,
	RANGING_INIT = 5,
	TYPE_ERROR = 254,
	RANGE_FAILED = 255,
};

#define LEN_DATA 90

// Max devices we put in the networkDevices array ! Each DW1000Device is 74 Bytes in SRAM memory for now.
#define MAX_DEVICES 12

// One blink every x polls
#define BLINK_INTERVAL 5

// Default Pin for module:
#define DEFAULT_RST_PIN 9
#define DEFAULT_SPI_SS_PIN 10
#define DEFAULT_SPI_IRQ_PIN 2

// Default value
// in ms
#define DEFAULT_RESET_PERIOD 2000
// in us
#define DEFAULT_REPLY_DELAY_TIME 3000

// sketch type (anchor or tag)
enum class BoardType : uint8_t
{
	TAG = 0,
	ANCHOR = 1,
};

// default timer delay
#define DEFAULT_RANGE_INTERVAL 500

#define ENABLE_RANGE_REPORT false

class DW1000Ranging
{
public:
	DW1000Ranging(PortableCode &_port) : _portable(_port), pDW1000(_port) {}
	// Initialization
	void init(BoardType type, uint16_t shortAddress, const char *wifiMacAddress, bool high_power, const uint8_t mode[], uint8_t myRST = DEFAULT_RST_PIN, uint8_t mySS = DEFAULT_SPI_SS_PIN, uint8_t myIRQ = DEFAULT_SPI_IRQ_PIN, float payload = 0.0);
	void init(BoardType type, const uint8_t *wifiMacAddress, uint16_t shortAddress, bool high_power, const uint8_t mode[], uint8_t myRST = DEFAULT_RST_PIN, uint8_t mySS = DEFAULT_SPI_SS_PIN, uint8_t myIRQ = DEFAULT_SPI_IRQ_PIN, float payload = 0.0);

	void loop();

	// Handlers
	void attachNewRange(void (*handleNewRange)(DW1000Device *)) { _handleNewRange = handleNewRange; };
	void attachRangeSent(void (*handleRangeSent)(DW1000Device *)) { _handleRangeSent = handleRangeSent; };
	void attachBlinkDevice(void (*handleBlinkDevice)(DW1000Device *)) { _handleBlinkDevice = handleBlinkDevice; };
	void attachNewDevice(void (*handleNewDevice)(DW1000Device *)) { _handleNewDevice = handleNewDevice; };
	void attachInactiveDevice(void (*handleInactiveDevice)(DW1000Device *)) { _handleInactiveDevice = handleInactiveDevice; };
	void attachRemovedDeviceMaxReached(void (*handleRemovedDeviceMaxReached)(DW1000Device *)) { _handleRemovedDeviceMaxReached = handleRemovedDeviceMaxReached; };
	void attachTimeoutExtReq(void (*requestTimeoutExtention)()) { _requestTimeoutExtention = requestTimeoutExtention; }

private:
	PortableCode &_portable;
	DW1000 pDW1000;

	// Initialization
	void initCommunication(uint8_t myRST, uint8_t mySS, uint8_t myIRQ);

	// variables
	// data buffer
	uint8_t receivedData[LEN_DATA];
	uint8_t sentData[LEN_DATA];

	// Initialization
	void configureNetwork(uint16_t deviceAddress, uint16_t networkId, const uint8_t mode[]);
	void generalStart(bool high_power);
	bool addNetworkDevices(DW1000Device *device);
	void removeNetworkDevices(uint8_t index);

	// Setters
	void setResetPeriod(uint32_t resetPeriod);

	// Getters
	uint8_t *getCurrentAddress() { return _ownLongAddress; };
	uint8_t *getCurrentShortAddress() { return _ownShortAddress; };
	uint8_t getNetworkDevicesNumber() { return _networkDevicesNumber; };

	// Utils
	MessageType detectMessageType(uint8_t datas[]);
	DW1000Device *searchDistantDevice(uint8_t shortAddress[]);
	void copyShortAddress(uint8_t address1[], uint8_t address2[]);

	// FOR DEBUGGING
	void visualizeDatas(uint8_t datas[]);

private:
	static constexpr const char *DW_RANGING = "dw1000_ranging";
	// Other devices in the network

	static constexpr short rangeDeviceSize = 16;
	static constexpr short pollDeviceSize = 4;
	static constexpr uint8_t pollAckTimeSlots = 6;
	static constexpr uint8_t devicePerPollTransmit = 4;

	std::vector<DW1000Device> _networkDevices;
	volatile uint8_t _networkDevicesNumber;
	uint8_t _ownLongAddress[8];
	uint8_t _ownShortAddress[2];
	uint8_t _lastSentToShortAddress[2];
	DW1000Mac _globalMac;
	uint32_t lastTimerTick;
	uint32_t _replyTimeOfLastPollAck;
	uint32_t _timeOfLastPollSent;
	float _payload;
	int16_t counterForBlink;

	// Handlers
	void (*_handleNewRange)(DW1000Device *);
	void (*_handleRangeSent)(DW1000Device *);
	void (*_handleBlinkDevice)(DW1000Device *);
	void (*_handleNewDevice)(DW1000Device *);
	void (*_handleInactiveDevice)(DW1000Device *);
	void (*_handleRemovedDeviceMaxReached)(DW1000Device *);
	void (*_requestTimeoutExtention)();

	// Board type (tag or anchor)
	BoardType _type;
	// Message sent/received state
	volatile bool _sentAck;
	volatile bool _receivedAck;
	// Protocol error state
	bool _protocolFailed;
	// Reset line to the chip
	uint8_t _RST;
	uint8_t _SS;
	// Watchdog and reset period
	uint32_t _lastActivity;
	uint32_t _resetPeriod;
	// Timer Tick delay
	uint16_t _timerDelay;
	// Millis between one range and another
	uint16_t _rangeInterval;
	// Ranging counter (per second)
	uint32_t _rangingCountPeriod;
	bool _first;

	// Methods
	void handleSent();
	void handleReceived();
	void noteActivity();
	void resetInactive();

	// Global functions:
	void checkForReset();
	void checkForInactiveDevices();

	// ANCHOR ranging protocol
	void transmitInit();
	void transmit(uint8_t datas[]);
	void transmit(uint8_t datas[], DW1000Time time);
	void transmitBlink();
	void transmitRangingInit(DW1000Device *myDistantDevice, uint16_t delay = 0);
	void transmitPollAck(DW1000Device *myDistantDevice, uint16_t delay);
	void transmitRangeReport(DW1000Device *myDistantDevice, uint16_t delay);
	void transmitRangeFailed(DW1000Device *myDistantDevice);
	void receiver();

	// TAG ranging protocol
	void transmitPoll();
	void transmitRange();

	// Methods for range computation
	void timerTick();
	void computeRangeAsymmetric(DW1000Device *myDistantDevice, DW1000Time *myTOF);
	uint16_t getReplyTimeOfIndex(int i);
};
