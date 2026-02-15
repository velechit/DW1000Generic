#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>
#include <functional>

#include "DW1000Constants.h"
#include "DW1000Time.h"

#include "portable.h"

#ifndef _BV
#define _BV(a) (1 << a)
#endif

class DW1000
{
public:
	void printSysStatus();

	DW1000(PortableCode &);

	/* ##### Init ################################################################ */
	/**
	Initiates and starts a sessions with one or more DW1000. If rst is not set or value 0xff, a soft resets (i.e. command
	triggered) are used and it is assumed that no reset line is wired.

	@param[in] irq The interrupt line/pin that connects the Arduino.
	@param[in] rst The reset line/pin for hard resets of ICs that connect to the Arduino. Value 0xff means soft reset.
	*/
	void begin();

	/**
	Selects a specific DW1000 chip for communication. In case of a single DW1000 chip in use
	this call only needs to be done once at start up, but is still mandatory. This function performs
	an initial setup of the now-selected chip.

	@param[in] ss The chip select line/pin that connects the to-be-selected chip with the
	Arduino.
	*/
	void select();

	/**
	Tells the driver library that no communication to a DW1000 will be required anymore.
	This basically just frees SPI and the previously used pins.
	*/
	void end();

	/**
	Enable debounce Clock, used to clock the LED blinking
	*/
	void enableDebounceClock();

	/**
	Enable led blinking feature
	*/
	void enableLedBlinking();

	/**
	Set GPIO mode
	*/
	void setGPIOMode(uint8_t msgp, uint8_t mode);

	/**
	Enable deep sleep mode
	*/
	void deepSleep();

	/**
	Wake-up from deep sleep by toggle chip select pin
	*/
	void spiWakeup();

	/**
	Resets all connected or the currently selected DW1000 chip. A hard reset of all chips
	is preferred, although a soft reset of the currently selected one is executed if no
	reset pin has been specified (when using `begin(int)`, instead of `begin(int, int)`).
	*/
	void reset(bool _soft = false);

	/**
	Resets the currently selected DW1000 chip programmatically (via corresponding commands).
	*/
	void softReset();

	/* ##### Print device id, address, etc. ###################################### */
	/**
	Generates a String representation of the device identifier of the chip. That usually
	are the letters "DECA" plus the	version and revision numbers of the chip.

	@param[out] msgBuffer The String buffer to be filled with printable device information.
		Provide 128 bytes, this should be sufficient.
	*/
	void getPrintableDeviceIdentifier(char msgBuffer[]);

	/**
	Generates a String representation of the extended unique identifier (EUI) of the chip.

	@param[out] msgBuffer The String buffer to be filled with printable device information.
		Provide 128 bytes, this should be sufficient.
	*/
	void getPrintableExtendedUniqueIdentifier(char msgBuffer[]);

	/**
	Generates a String representation of the short address and network identifier currently
	defined for the respective chip.

	@param[out] msgBuffer The String buffer to be filled with printable device information.
		Provide 128 bytes, this should be sufficient.
	*/
	void getPrintableNetworkIdAndShortAddress(char msgBuffer[]);

	/**
	Generates a String representation of the main operational settings of the chip. This
	includes data rate, pulse repetition frequency, preamble and channel settings.

	@param[out] msgBuffer The String buffer to be filled with printable device information.
		Provide 128 bytes, this should be sufficient.
	*/
	void getPrintableDeviceMode(char msgBuffer[]);

	/* ##### Device address management, filters ################################## */
	/**
	(Re-)set the network identifier which the selected chip should be associated with. This
	setting is important for certain MAC address filtering rules.

	@param[in] val An arbitrary numeric network identifier.
	*/

	void setNetworkId(uint16_t val);

	/**
	(Re-)set the device address (i.e. short address) for the currently selected chip. This
	setting is important for certain MAC address filtering rules.

	@param[in] val An arbitrary numeric device address.
	*/
	void setDeviceAddress(uint16_t val);
	// TODO MAC and filters

	void setEUI(uint8_t eui[]);

	/* ##### General device configuration ######################################## */
	/**
	Specifies whether the DW1000 chip should, again, turn on its receiver in case that the
	last reception failed.

	This setting is enabled as part of `setDefaults()` if the device is
	in idle mode.

	@param[in] val `true` to enable, `false` to disable receiver auto-reenable.
	*/
	void setReceiverAutoReenable(bool val);

	/**
	Specifies the interrupt polarity of the DW1000 chip.

	As part of `setDefaults()` if the device is in idle mode, interrupt polarity is set to
	active high.

	@param[in] val `true` for active high interrupts, `false` for active low interrupts.
	*/
	void setInterruptPolarity(bool val);

	/**
	Specifies whether to suppress any frame check measures while sending or receiving messages.
	If suppressed, no 2-uint8_t checksum is appended to the message before sending and this
	checksum is also not expected at receiver side. Note that when suppressing frame checks,
	the error event handler	(attached via `attachReceiveErrorHandler()`) will not be triggered
	if received data is corrupted.

	Frame checks are enabled as part of `setDefaults()` if the device is in idle mode.

	@param[in] val `true` to suppress frame check on sender and receiver side, `false` otherwise.
	*/
	void suppressFrameCheck(bool val);

	/**
	Specifies the data transmission rate of the DW1000 chip. One of the values
	- `TRX_RATE_110KBPS` (i.e. 110 kb/s)
	- `TRX_RATE_850KBPS` (i.e. 850 kb/s)
	- `TRX_RATE_6800KBPS` (i.e. 6.8 Mb/s)
	has to be provided.

	See `setDefaults()` and `enableMode()` for additional information on data rate settings.

	@param[in] rate The data transmission rate, encoded by the above defined constants.
	*/
	void setDataRate(uint8_t rate);

	/**
	Specifies the pulse repetition frequency (PRF) of data transmissions with the DW1000. Either
	- `TX_PULSE_FREQ_16MHZ` (i.e. 16 MHz)
	- `TX_PULSE_FREQ_64MHZ` (i.e. 64 MHz)
	has to be chosen.

	Note that the 16 MHz setting is more power efficient, while the 64 MHz setting requires more
	power, but also delivers slightly better transmission performance (i.e. on communication range and
	timestamp accuracy) (see DWM1000 User Manual, section 9.3).

	See `setDefaults()` and `enableMode()` for additional information on PRF settings.

	@param[in] freq The PRF, encoded by the above defined constants.
	*/
	void setPulseFrequency(uint8_t freq);
	uint8_t getPulseFrequency();
	void setPreambleLength(uint8_t prealen);
	void setChannel(uint8_t channel);
	void setPreambleCode(uint8_t preacode);
	void useSmartPower(bool smartPower);

	/* transmit and receive configuration. */
	DW1000Time setDelay(const DW1000Time &delay);
	void receivePermanently(bool val);
	void setData(uint8_t data[], uint16_t n);
	void setData(const std::string &data);
	void getData(uint8_t data[], uint16_t n);
	void getData(std::string &data);
	uint16_t getDataLength();
	void getTransmitTimestamp(DW1000Time &time);
	void getReceiveTimestamp(DW1000Time &time);
	void getSystemTimestamp(DW1000Time &time);
	void getTransmitTimestamp(uint8_t data[]);
	void getReceiveTimestamp(uint8_t data[]);
	void getSystemTimestamp(uint8_t data[]);

	/* receive quality information. */
	float getReceivePower();
	float getFirstPathPower();
	float getReceiveQuality();

	/* interrupt management. */
	void interruptOnSent(bool val);
	void interruptOnReceived(bool val);
	void interruptOnReceiveFailed(bool val);
	void interruptOnReceiveTimeout(bool val);
	void interruptOnReceiveTimestampAvailable(bool val);
	void interruptOnAutomaticAcknowledgeTrigger(bool val);

	/* Antenna delay calibration */
	void setAntennaDelay(const uint16_t value);
	uint16_t getAntennaDelay();

	/* callback handler management. */
	void attachErrorHandler(std::function<void()> handleError)
	{
		_handleError = handleError;
	}

	void attachSentHandler(std::function<void()> handleSent)
	{
		_handleSent = handleSent;
	}

	void attachReceivedHandler(std::function<void()> handleReceived)
	{
		_handleReceived = handleReceived;
	}

	void attachReceiveFailedHandler(std::function<void()> handleReceiveFailed)
	{
		_handleReceiveFailed = handleReceiveFailed;
	}

	void attachReceiveTimeoutHandler(std::function<void()> handleReceiveTimeout)
	{
		_handleReceiveTimeout = handleReceiveTimeout;
	}

	void attachReceiveTimestampAvailableHandler(std::function<void()> handleReceiveTimestampAvailable)
	{
		_handleReceiveTimestampAvailable = handleReceiveTimestampAvailable;
	}

	/* device state management. */
	// idle state
	void idle();

	// general configuration state
	void newConfiguration();
	void commitConfiguration();

	// reception state
	void newReceive();
	void startReceive();

	// transmission state
	void newTransmit();
	void startTransmit();

	// Vincent changes
	// For large power moudle
	void high_power_init();

	/* ##### Operation mode selection ############################################ */
	/**
	Specifies the mode of operation for the DW1000. Modes of operation are pre-defined
	combinations of data rate, pulse repetition frequency, preamble and channel settings
	that properly go together. If you simply want the chips to work, choosing a mode is
	preferred over manual configuration.

	The following modes are pre-configured and one of them needs to be chosen:
	- `MODE_LONGDATA_RANGE_LOWPOWER` (basically this is 110 kb/s data rate, 16 MHz PRF and long preambles)
	- `MODE_SHORTDATA_FAST_LOWPOWER` (basically this is 6.8 Mb/s data rate, 16 MHz PRF and short preambles)
	- `MODE_LONGDATA_FAST_LOWPOWER` (basically this is 6.8 Mb/s data rate, 16 MHz PRF and long preambles)
	- `MODE_SHORTDATA_FAST_ACCURACY` (basically this is 6.8 Mb/s data rate, 64 MHz PRF and short preambles)
	- `MODE_LONGDATA_FAST_ACCURACY` (basically this is 6.8 Mb/s data rate, 64 MHz PRF and long preambles)
	- `MODE_LONGDATA_RANGE_ACCURACY` (basically this is 110 kb/s data rate, 64 MHz PRF and long preambles)

	Note that LOWPOWER and ACCURACY refers to the better power efficiency and improved transmission performance
	of 16 MHZ and 64 MHZ PRF respectively (see `setPulseFrequency()`).

	The default setting that is selected by `setDefaults()` is MODE_LONGDATA_RANGE_LOWPOWER.

	@param[in] mode The mode of operation, encoded by the above defined constants.
	*/
	void enableMode(const uint8_t mode[]);

	// use RX/TX specific and general default settings
	void setDefaults();

	/* debug pretty print registers. */
	void getPrettyBytes(uint8_t cmd, uint16_t offset, char msgBuffer[], uint16_t n);
	void getPrettyBytes(uint8_t data[], char msgBuffer[], uint16_t n);

	// convert from char to 4 bits (hexadecimal)
	uint8_t nibbleFromChar(char c);
	void convertToByte(const char string[], uint8_t *eui_byte, uint8_t size = LEN_EUI);
	void convertToByte(uint16_t val, uint8_t *bytes);

	// host-initiated reading of temperature and battery voltage
	void getTempAndVbat(float &temp, float &vbat);

	// transmission/reception bit rate
	static constexpr uint8_t TRX_RATE_110KBPS = 0x00;
	static constexpr uint8_t TRX_RATE_850KBPS = 0x01;
	static constexpr uint8_t TRX_RATE_6800KBPS = 0x02;

	// transmission pulse frequency
	// 0x00 is 4MHZ, but receiver in DW1000 does not support it (!??)
	static constexpr uint8_t TX_PULSE_FREQ_16MHZ = 0x01;
	static constexpr uint8_t TX_PULSE_FREQ_64MHZ = 0x02;

	// preamble length (PE + TXPSR bits)
	static constexpr uint8_t TX_PREAMBLE_LEN_64 = 0x01;
	static constexpr uint8_t TX_PREAMBLE_LEN_128 = 0x05;
	static constexpr uint8_t TX_PREAMBLE_LEN_256 = 0x09;
	static constexpr uint8_t TX_PREAMBLE_LEN_512 = 0x0D;
	static constexpr uint8_t TX_PREAMBLE_LEN_1024 = 0x02;
	static constexpr uint8_t TX_PREAMBLE_LEN_1536 = 0x06;
	static constexpr uint8_t TX_PREAMBLE_LEN_2048 = 0x0A;
	static constexpr uint8_t TX_PREAMBLE_LEN_4096 = 0x03;

	// PAC size. */
	static constexpr uint8_t PAC_SIZE_8 = 8;
	static constexpr uint8_t PAC_SIZE_16 = 16;
	static constexpr uint8_t PAC_SIZE_32 = 32;
	static constexpr uint8_t PAC_SIZE_64 = 64;

	/* channel of operation. */
	static constexpr uint8_t CHANNEL_1 = 1;
	static constexpr uint8_t CHANNEL_2 = 2;
	static constexpr uint8_t CHANNEL_3 = 3;
	static constexpr uint8_t CHANNEL_4 = 4;
	static constexpr uint8_t CHANNEL_5 = 5;
	static constexpr uint8_t CHANNEL_7 = 7;

	/* preamble codes. */
	static constexpr uint8_t PREAMBLE_CODE_16MHZ_1 = 1;
	static constexpr uint8_t PREAMBLE_CODE_16MHZ_2 = 2;
	static constexpr uint8_t PREAMBLE_CODE_16MHZ_3 = 3;
	static constexpr uint8_t PREAMBLE_CODE_16MHZ_4 = 4;
	static constexpr uint8_t PREAMBLE_CODE_16MHZ_5 = 5;
	static constexpr uint8_t PREAMBLE_CODE_16MHZ_6 = 6;
	static constexpr uint8_t PREAMBLE_CODE_16MHZ_7 = 7;
	static constexpr uint8_t PREAMBLE_CODE_16MHZ_8 = 8;
	static constexpr uint8_t PREAMBLE_CODE_64MHZ_9 = 9;
	static constexpr uint8_t PREAMBLE_CODE_64MHZ_10 = 10;
	static constexpr uint8_t PREAMBLE_CODE_64MHZ_11 = 11;
	static constexpr uint8_t PREAMBLE_CODE_64MHZ_12 = 12;
	static constexpr uint8_t PREAMBLE_CODE_64MHZ_17 = 17;
	static constexpr uint8_t PREAMBLE_CODE_64MHZ_18 = 18;
	static constexpr uint8_t PREAMBLE_CODE_64MHZ_19 = 19;
	static constexpr uint8_t PREAMBLE_CODE_64MHZ_20 = 20;

	/* frame length settings. */
	static constexpr uint8_t FRAME_LENGTH_NORMAL = 0x00;
	static constexpr uint8_t FRAME_LENGTH_EXTENDED = 0x03;

	/* pre-defined modes of operation (3 bytes for data rate, pulse frequency and
	preamble length). */
	static constexpr uint8_t MODE_LONGDATA_RANGE_LOWPOWER[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_2048};
	static constexpr uint8_t MODE_SHORTDATA_FAST_LOWPOWER[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_128};
	static constexpr uint8_t MODE_LONGDATA_FAST_LOWPOWER[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_1024};
	static constexpr uint8_t MODE_SHORTDATA_FAST_ACCURACY[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_128};
	static constexpr uint8_t MODE_LONGDATA_FAST_ACCURACY[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_1024};
	static constexpr uint8_t MODE_LONGDATA_RANGE_ACCURACY[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_2048};

private:
	PortableCode &_portable;

	/* callbacks. */
	std::function<void()> _handleError;
	std::function<void()> _handleSent;
	std::function<void()> _handleReceived;
	std::function<void()> _handleReceiveFailed;
	std::function<void()> _handleReceiveTimeout;
	std::function<void()> _handleReceiveTimestampAvailable;

	/* register caches. */
	uint8_t _syscfg[LEN_SYS_CFG];
	uint8_t _sysctrl[LEN_SYS_CTRL];
	uint8_t _sysstatus[LEN_SYS_STATUS];
	uint8_t _txfctrl[LEN_TX_FCTRL];
	uint8_t _sysmask[LEN_SYS_MASK];
	uint8_t _chanctrl[LEN_CHAN_CTRL];

	/* device status monitoring */
	uint8_t _vmeas3v3;
	uint8_t _tmeas23C;

	/* PAN and short address. */
	uint8_t _networkAndAddress[LEN_PANADR];

	/* internal helper that guide tuning the chip. */
	bool _smartPower;
	uint8_t _extendedFrameLength;
	uint8_t _preambleCode;
	uint8_t _channel;
	uint8_t _preambleLength;
	uint8_t _pulseFrequency;
	uint8_t _dataRate;
	uint8_t _pacSize;
	DW1000Time _antennaDelay;
	bool _antennaCalibrated;

	/* internal helper to remember how to properly act. */
	bool _permanentReceive;
	bool _frameCheck;

	// whether RX or TX is active
	uint8_t _deviceMode;

	// whether debounce clock is active
	bool _debounceClockEnabled;

	/* Arduino interrupt handler */
	void handleInterrupt();

	/* Allow MAC frame filtering . */
	// TODO auto-acknowledge
	void setFrameFilter(bool val);
	void setFrameFilterBehaveCoordinator(bool val);
	void setFrameFilterAllowBeacon(bool val);
	// data type is used in the FC_1 0x41
	void setFrameFilterAllowData(bool val);
	void setFrameFilterAllowAcknowledgement(bool val);
	void setFrameFilterAllowMAC(bool val);
	// Reserved is used for the Blink message
	void setFrameFilterAllowReserved(bool val);

	// note: not sure if going to be implemented for now
	void setDoubleBuffering(bool val);
	// TODO is implemented, but needs testing
	void useExtendedFrameLength(bool val);
	// TODO is implemented, but needs testing
	void waitForResponse(bool val);

	/* tuning according to mode. */
	void tune();

	/* device status flags */
	bool isReceiveTimestampAvailable();
	bool isTransmitDone();
	bool isReceiveDone();
	bool isReceiveFailed();
	bool isReceiveTimeout();
	bool isClockProblem();

	/* interrupt state handling */
	void clearInterrupts();
	void clearAllStatus();
	void clearReceiveStatus();
	void clearReceiveTimestampAvailableStatus();
	void clearTransmitStatus();

	/* internal helper to read/write system registers. */
	void readSystemEventStatusRegister();
	void readSystemConfigurationRegister();
	void writeSystemConfigurationRegister();
	void readNetworkIdAndDeviceAddress();
	void writeNetworkIdAndDeviceAddress();
	void readSystemEventMaskRegister();
	void writeSystemEventMaskRegister();
	void readChannelControlRegister();
	void writeChannelControlRegister();
	void readTransmitFrameControlRegister();
	void writeTransmitFrameControlRegister();

	/* clock management. */
	void enableClock(uint8_t clock);

	/* LDE micro-code management. */
	void manageLDE();

	/* timestamp correction. */
	void correctTimestamp(DW1000Time &timestamp);

	/* reading and writing bytes from and to DW1000 module. */
	void readBytes(uint8_t cmd, uint16_t offset, uint8_t data[], uint16_t n);
	void readBytesOTP(uint16_t address, uint8_t data[]);
	void writeByte(uint8_t cmd, uint16_t offset, uint8_t data);
	void writeBytes(uint8_t cmd, uint16_t offset, uint8_t data[], uint16_t n);

	/* writing numeric values to bytes. */
	void writeValueToBytes(uint8_t data[], int32_t val, uint16_t n);

	/* internal helper for bit operations on multi-bytes. */
	bool getBit(uint8_t data[], uint16_t n, uint16_t bit);
	void setBit(uint8_t data[], uint16_t n, uint16_t bit, bool val);

	/* Register is 6 bit, 7 = write, 6 = sub-adressing, 5-0 = register value
	 * Total header with sub-adressing can be 15 bit. */
	static constexpr uint8_t WRITE = 0x80;		// regular write
	static constexpr uint8_t WRITE_SUB = 0xC0;	// write with sub address
	static constexpr uint8_t READ = 0x00;		// regular read
	static constexpr uint8_t READ_SUB = 0x40;	// read with sub address
	static constexpr uint8_t RW_SUB_EXT = 0x80; // R/W with sub address extension

	/* clocks available. */
	static constexpr uint8_t AUTO_CLOCK = 0x00;
	static constexpr uint8_t XTI_CLOCK = 0x01;
	static constexpr uint8_t PLL_CLOCK = 0x02;

	/* range bias tables (500/900 MHz band, 16/64 MHz PRF), -61 to -95 dBm. */
	static constexpr uint8_t BIAS_500_16_ZERO = 10;
	static constexpr uint8_t BIAS_500_64_ZERO = 8;
	static constexpr uint8_t BIAS_900_16_ZERO = 7;
	static constexpr uint8_t BIAS_900_64_ZERO = 7;

	// range bias tables (500 MHz in [mm] and 900 MHz in [2mm] - to fit into bytes)
	static constexpr uint8_t BIAS_500_16[] = {198, 187, 179, 163, 143, 127, 109, 84, 59, 31, 0, 36, 65, 84, 97, 106, 110, 112};
	static constexpr uint8_t BIAS_500_64[] = {110, 105, 100, 93, 82, 69, 51, 27, 0, 21, 35, 42, 49, 62, 71, 76, 81, 86};
	static constexpr uint8_t BIAS_900_16[] = {137, 122, 105, 88, 69, 47, 25, 0, 21, 48, 79, 105, 127, 147, 160, 169, 178, 197};
	static constexpr uint8_t BIAS_900_64[] = {147, 133, 117, 99, 75, 50, 29, 0, 24, 45, 63, 76, 87, 98, 116, 122, 132, 142};
};
