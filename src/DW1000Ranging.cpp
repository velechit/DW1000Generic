
#include "DW1000Ranging.h"
#include "DW1000Device.h"

void DW1000Ranging::init(BoardType type, uint16_t shortAddress, const char *wifiMacAddress, bool high_power, const uint8_t mode[], uint8_t myRST, uint8_t mySS, uint8_t myIRQ, float payload)
{
	uint8_t byteWifiMacAddress[6] = {0};
	pDW1000.convertToByte(wifiMacAddress, byteWifiMacAddress, 6);
	init(type, byteWifiMacAddress, shortAddress, high_power, mode, myRST, mySS, myIRQ, payload);
}

void DW1000Ranging::init(BoardType type, const uint8_t *wifiMacAddress, uint16_t shortAddress, bool high_power, const uint8_t mode[], uint8_t myRST, uint8_t mySS, uint8_t myIRQ, float payload)
{
	for (int i = 0; i < MAX_DEVICES; i++)
		_networkDevices.push_back(DW1000Device(_portable));

	_networkDevicesNumber = 0;
	_sentAck = false;
	_receivedAck = false;
	_protocolFailed = false;
	lastTimerTick = 0;
	_replyTimeOfLastPollAck = 0;
	_timeOfLastPollSent = 0;
	counterForBlink = 0; // TODO 8 bit?
	_rangeInterval = DEFAULT_RANGE_INTERVAL;
	_rangingCountPeriod = 0;
	_handleNewRange = 0;
	_handleRangeSent = 0;
	_handleBlinkDevice = 0;
	_handleNewDevice = 0;
	_handleInactiveDevice = 0;
	_handleRemovedDeviceMaxReached = 0;
	_payload = payload;
	_requestTimeoutExtention = 0;
	_first = true;

	initCommunication(myRST, mySS, myIRQ);

	// convert the address
	pDW1000.convertToByte(shortAddress, _ownLongAddress);
	// pDW1000.convertToByte(wifiMacAddress, _ownLongAddress + 2, 6);
	_ownLongAddress[2] = wifiMacAddress[0];
	_ownLongAddress[3] = wifiMacAddress[1];
	_ownLongAddress[4] = wifiMacAddress[2];
	_ownLongAddress[5] = wifiMacAddress[3];
	_ownLongAddress[6] = wifiMacAddress[4];
	_ownLongAddress[7] = wifiMacAddress[5];

	// we use first two bytes in address for short address
	_ownShortAddress[0] = _ownLongAddress[0];
	_ownShortAddress[1] = _ownLongAddress[1];

	// write the address on the DW1000 chip
	pDW1000.setEUI(_ownLongAddress);

	// we configure the network for mac filtering
	//(device Address, network ID, frequency)
	configureNetwork(shortAddress, 0xDECA, mode);

	// general start
	generalStart(high_power);

	// defined type
	_type = type;

	if (_type == BoardType::ANCHOR)
		_portable.log_inf(DW_RANGING, "### ANCHOR ###");
	else if (type == BoardType::TAG)
		_portable.log_inf(DW_RANGING, "### TAG ###");

	char msg[6];
	sprintf(msg, "%02X:%02X", _ownShortAddress[0], _ownShortAddress[1]);
	_portable.log_inf(DW_RANGING, "Short address: %s", msg);
}

/* ###########################################################################
 * #### Init and end #########################################################
 * ########################################################################### */

void DW1000Ranging::initCommunication(uint8_t myRST, uint8_t mySS, uint8_t myIRQ)
{
	// reset line to the chip
	_RST = myRST;
	_SS = mySS;
	_resetPeriod = DEFAULT_RESET_PERIOD;

	// we set our timer delay
	_timerDelay = _rangeInterval;

	pDW1000.begin();
}

void DW1000Ranging::configureNetwork(uint16_t deviceAddress, uint16_t networkId, const uint8_t mode[])
{
	// general configuration
	pDW1000.newConfiguration();
	pDW1000.setDefaults();
	pDW1000.setDeviceAddress(deviceAddress);
	pDW1000.setNetworkId(networkId);
	pDW1000.enableMode(mode);
	pDW1000.commitConfiguration();
}

void DW1000Ranging::generalStart(bool high_power)
{
	// attach callback for (successfully) sent and received messages
	pDW1000.attachSentHandler([&]()
							  { handleSent(); });
	pDW1000.attachReceivedHandler([&]()
								  { handleReceived(); });
	// anchor starts in receiving mode, awaiting a ranging poll message

	if (high_power)
		pDW1000.high_power_init();

	// anchor starts in receiving mode, awaiting a ranging poll message
	receiver();
	// for first time ranging frequency computation
	_rangingCountPeriod = _portable.millis();
}

bool DW1000Ranging::addNetworkDevices(DW1000Device *device)
{
	// if (device->getShortAddress() == 114 ||
	// 	device->getShortAddress() == 115)
	// 	return true;

	// we test our network devices array to check
	// we don't already have it
	uint8_t worstQuality = 0;
	for (uint8_t i = 0; i < _networkDevicesNumber; i++)
	{
		if (_networkDevices[i].isShortAddressEqual(device))
			return false; // the device already exists

		if (_networkDevices[i].getQuality() < _networkDevices[worstQuality].getQuality())
			worstQuality = i;
	}

	device->setRange(0);
	if (_networkDevicesNumber < MAX_DEVICES)
	{
		memcpy((void *)&_networkDevices[_networkDevicesNumber], device, sizeof(DW1000Device));
		_networkDevices[_networkDevicesNumber].setIndex(_networkDevicesNumber);
		_networkDevicesNumber += 1;
	}
	else
	{
		// Reached max devices count, replace the worst (farthest) one
		if (_handleRemovedDeviceMaxReached != 0)
		{
			(*_handleRemovedDeviceMaxReached)(&_networkDevices[worstQuality]);
		}
		memcpy((void *)&_networkDevices[worstQuality], device, sizeof(DW1000Device));
		_networkDevices[worstQuality].setIndex(worstQuality);
	}
	return true;
}

void DW1000Ranging::removeNetworkDevices(uint8_t index)
{
	if (index != _networkDevicesNumber - 1) // if we do not delete the last element
	{
		// we replace the element we want to delete with the last one
		memcpy((void *)&_networkDevices[index], &_networkDevices[_networkDevicesNumber - 1], sizeof(DW1000Device));
		_networkDevices[index].setIndex(index);
	}
	_networkDevicesNumber -= 1;
}

/* ###########################################################################
 * #### Setters and Getters ##################################################
 * ########################################################################### */

// setters
void DW1000Ranging::setResetPeriod(uint32_t resetPeriod) { _resetPeriod = resetPeriod; }

DW1000Device *DW1000Ranging::searchDistantDevice(uint8_t shortAddress[])
{
	// we compare the 2 bytes address with the others
	for (uint8_t i = 0; i < _networkDevicesNumber; i++)
	{
		if (memcmp(shortAddress, _networkDevices[i].getByteShortAddress(), 2) == 0)
		{
			// we have found our device !
			return &_networkDevices[i];
		}
	}

	return nullptr;
}

/* ###########################################################################
 * #### Public methods #######################################################
 * ########################################################################### */

void DW1000Ranging::checkForReset()
{
	if (!_sentAck && !_receivedAck)
	{
		resetInactive();
		return; // TODO cc
	}
}

void DW1000Ranging::checkForInactiveDevices()
{
	uint8_t inactiveDevicesNum = 0;
	uint8_t inactiveDevices[MAX_DEVICES];
	for (uint8_t i = 0; i < _networkDevicesNumber; i++)
	{
		if (_networkDevices[i].isInactive())
		{
			inactiveDevices[inactiveDevicesNum++] = i;
			if (_handleInactiveDevice != 0)
			{
				(*_handleInactiveDevice)(&_networkDevices[i]);
			}
		}
	}

	// we need to delete the device from the array:
	for (uint8_t i = 0; i < inactiveDevicesNum; i++)
	{
		removeNetworkDevices(inactiveDevices[i]);
	}
}

MessageType DW1000Ranging::detectMessageType(uint8_t datas[])
{
	if (datas[0] == FC_1_BLINK)
	{
		return MessageType::BLINK;
	}
	else if (datas[0] == FC_1 && datas[1] == FC_2)
	{
		// we have a long MAC frame message (ranging init)
		return static_cast<MessageType>(datas[LONG_MAC_LEN]);
	}
	else if (datas[0] == FC_1 && datas[1] == FC_2_SHORT)
	{
		// we have a short mac frame message (poll, range, range report, etc..)
		return static_cast<MessageType>(datas[SHORT_MAC_LEN]);
	}
	return MessageType::TYPE_ERROR;
}

uint32_t DEBUGtimePollSent;
uint32_t DEBUGRangeSent;

void DW1000Ranging::loop()
{
	// we check if needed to reset!
	checkForReset();
	uint32_t currentTime = _portable.millis();

	if (currentTime - lastTimerTick > _timerDelay)
	{
		lastTimerTick = currentTime;
		timerTick();
	}
	if (!_sentAck && !_receivedAck)
	{
		if (_replyTimeOfLastPollAck != 0 && currentTime - _timeOfLastPollSent > _replyTimeOfLastPollAck + 3)
		{

			if (_type == BoardType::TAG && _first)
			{
				_portable.log_inf(DW_RANGING, "Its quite long anyone sent ACK, reset DWM", _networkDevicesNumber);
				pDW1000.select();
				_first = false;
				if (_requestTimeoutExtention != 0)
					(*_requestTimeoutExtention)();
				// we may need to ask extention of entering sleep???

				// receiver(); // may be we were deaf that is why we could not hear anyone ???
			}

			//			_portable.log_inf(DW_RANGING,  "RANGE ON TIMEOUT");
			//			transmitRange();
		}
	}
	if (_sentAck)
	{
		_sentAck = false;

		MessageType messageType = detectMessageType(sentData);
		switch (messageType)
		{
		case MessageType::POLL:
			_portable.log_dbg(DW_RANGING, "POLL");
			break;
		case MessageType::POLL_ACK:
			_portable.log_dbg(DW_RANGING, "POLL_ACK");
			break;
		case MessageType::RANGE:
			_portable.log_dbg(DW_RANGING, "RANGE");
			break;
		case MessageType::RANGE_REPORT:
			_portable.log_dbg(DW_RANGING, "RANGE_REPORT");
			break;
		case MessageType::BLINK:
			_portable.log_dbg(DW_RANGING, "BLINK");
			break;
		case MessageType::RANGING_INIT:
			_portable.log_dbg(DW_RANGING, "RANGING_INIT");
			break;
		case MessageType::TYPE_ERROR:
			_portable.log_dbg(DW_RANGING, "TYPE_ERROR");
			break;
		case MessageType::RANGE_FAILED:
			_portable.log_dbg(DW_RANGING, "RANGE_FAILED");
			break;
		};

		if (messageType != MessageType::POLL_ACK && messageType != MessageType::POLL && messageType != MessageType::RANGE)
			return;

		// A msg was sent. We launch the ranging protocol when a message was sent
		if (_type == BoardType::ANCHOR && messageType == MessageType::POLL_ACK)
		{
			uint8_t tagAddr[2];
			tagAddr[0] = sentData[6];
			tagAddr[1] = sentData[5];
			_portable.log_inf(DW_RANGING, "ACK sent to %02x:%02x", tagAddr[0], tagAddr[1]);

			DW1000Device *myDistantDevice = searchDistantDevice(tagAddr);
			if (myDistantDevice)
				pDW1000.getTransmitTimestamp(myDistantDevice->timePollAckSent);
		}
		else if (_type == BoardType::TAG)
		{
			if (messageType == MessageType::BLINK)
			{
				receiver();
			}
			else if (messageType == MessageType::POLL)
			{
				DW1000Time timePollSent;
				pDW1000.getTransmitTimestamp(timePollSent);

				// we save the value for all the devices !
				// TODO: check the devices in the sent POLL message and mark only them
				for (uint8_t i = 0; i < _networkDevicesNumber; i++)
				{
					_networkDevices[i].timePollSent = timePollSent;
					_networkDevices[i].hasSentPollAck = false;
					_networkDevices[i].hasRangeBeenServed = false;
				}
				receiver();
			}
			else if (messageType == MessageType::RANGE)
			{
				DW1000Time timeRangeSent;
				pDW1000.getTransmitTimestamp(timeRangeSent);
				// we save the value for all the devices !
				for (uint8_t i = 0; i < _networkDevicesNumber; i++)
				{
					_networkDevices[i].timeRangeSent = timeRangeSent;
					if (_handleRangeSent != 0)
						(*_handleRangeSent)(&_networkDevices[i]);
				}
			}
		}
	}

	// check for new received message
	if (_receivedAck)
	{
		_receivedAck = false;

		// we read the datas from the modules:
		//  get message and parse
		pDW1000.getData(receivedData, LEN_DATA);

		MessageType messageType = detectMessageType(receivedData);

		switch (messageType)
		{
		case MessageType::POLL:
			_portable.log_dbg(DW_RANGING, "<=POLL");
			break;
		case MessageType::POLL_ACK:
			_portable.log_dbg(DW_RANGING, "<=POLL_ACK");
			break;
		case MessageType::RANGE:
			_portable.log_dbg(DW_RANGING, "<=RANGE");
			break;
		case MessageType::RANGE_REPORT:
			_portable.log_dbg(DW_RANGING, "<=RANGE_REPORT");
			break;
		case MessageType::BLINK:
			_portable.log_dbg(DW_RANGING, "<=BLINK");
			break;
		case MessageType::RANGING_INIT:
			_portable.log_dbg(DW_RANGING, "<=RANGING_INIT");
			break;
		case MessageType::TYPE_ERROR:
			_portable.log_dbg(DW_RANGING, "<=TYPE_ERROR");
			break;
		case MessageType::RANGE_FAILED:
			_portable.log_dbg(DW_RANGING, "<=RANGE_FAILED");
			break;
		};

		// we have just received a BLINK message from tag
		if (messageType == MessageType::BLINK && _type == BoardType::ANCHOR) // At the ANCHOR side, we received this we check if TAG knows us, if not, we send a RANGING_INIT
		{
			uint8_t tagAddr[2];
			_globalMac.decodeBlinkFrame(receivedData, tagAddr);

			bool knownByTheTag = false;

			uint8_t numberDevices = receivedData[BLINK_MAC_LEN];
			for (uint8_t i = 0; i < numberDevices; i++)
			{
				// we check if the tag know us
				uint8_t _blinkAnchorShortAddr[2];
				memcpy(_blinkAnchorShortAddr, receivedData + BLINK_MAC_LEN + 1 + i * 2, 2);
				// we test if the short address is our address
				if (_blinkAnchorShortAddr[0] == _ownShortAddress[0] &&
					_blinkAnchorShortAddr[1] == _ownShortAddress[1])
					knownByTheTag = true;
			}

			DW1000Device myTag(_portable, tagAddr);
			myTag.setRXPower(pDW1000.getReceivePower());
			myTag.setFPPower(pDW1000.getFirstPathPower());
			myTag.setQuality(pDW1000.getReceiveQuality());

			if (_handleBlinkDevice != 0)
			{
				// we create a new device with the tag
				(*_handleBlinkDevice)(&myTag);
			}

			if (!knownByTheTag) // if TAG does not know us, ask it to notedown by sending a RANGING_INIT
			{
				// we reply by the transmit ranging init message
				_portable.log_vrb(DW_RANGING, "Sending RANGING_INIT to %02x:%02x", tagAddr[0], tagAddr[1]);
				constexpr short slotQty = 7;
				constexpr uint16_t slotDuration = 2.5 * DEFAULT_REPLY_DELAY_TIME;
				int randomSlot = _portable.random(0, slotQty) + 1;
				uint16_t delay = slotDuration * randomSlot;
				transmitRangingInit(&myTag, delay); // RANGING_INIT as unicast to only that TAG
			}

			noteActivity();
		}
		else if (messageType == MessageType::RANGING_INIT && _type == BoardType::TAG) // At the TAG side, we received this as anchor want us to take note of it, for case when we send next POLL or BLINK
		{
			if (receivedData[6] != _ownShortAddress[0] || receivedData[5] != _ownShortAddress[1]) // if this TAG was not the receiver of this UNICAST frame, ignore it
				return;

			uint8_t address[2];
			_globalMac.decodeShortMACFrame(receivedData, address);
			// we crate a new device with the anchor
			DW1000Device myAnchor(_portable, address);
			myAnchor.setRXPower(pDW1000.getReceivePower());
			myAnchor.setFPPower(pDW1000.getFirstPathPower());
			myAnchor.setQuality(pDW1000.getReceiveQuality());

			_portable.log_vrb(DW_RANGING, "RANGING_INIT from %x", myAnchor.getShortAddress());

			if (addNetworkDevices(&myAnchor))
				if (_handleNewDevice != 0)
					(*_handleNewDevice)(&myAnchor);

			noteActivity();
		}
		else
		{
			// we have a short mac layer frame !
			uint8_t address[2];
			_globalMac.decodeShortMACFrame(receivedData, address);

			// we get the device which correspond to the message which was sent (need to be filtered by MAC address)
			//	DW1000Device *myDistantDevice = searchDistantDevice(address);

			// then we proceed to range protocol
			if (_type == BoardType::ANCHOR)
			{
				if (messageType == MessageType::POLL)
				{
					// we receive a POLL which is a broadcast message
					// we need to grab info about it
					DW1000Time timePollReceived;
					pDW1000.getReceiveTimestamp(timePollReceived);

					uint8_t numberDevices = receivedData[SHORT_MAC_LEN + 1];

					for (uint8_t i = 0; i < numberDevices; i++)
					{
						uint8_t shortAddress[2];
						memcpy(shortAddress, receivedData + SHORT_MAC_LEN + 2 + i * pollDeviceSize, 2);

						// we test if the short address is our address
						if (shortAddress[0] == _ownShortAddress[0] &&
							shortAddress[1] == _ownShortAddress[1])
						{
							// a poll message from a tag with our ID in it
							// so we need to store it
							// we create a new device with the tag
							DW1000Device *myDistantDevice = searchDistantDevice(address);
							if (myDistantDevice == nullptr)
							{
								_portable.log_inf(DW_RANGING, "Device %x:%x added to database", address[0], address[1]);
								DW1000Device myTag(_portable, address);
								myTag.setRXPower(pDW1000.getReceivePower());
								myTag.setFPPower(pDW1000.getFirstPathPower());
								myTag.setQuality(pDW1000.getReceiveQuality());
								addNetworkDevices(&myTag); // and store it
								myDistantDevice = searchDistantDevice(address);
							}

							myDistantDevice->noteActivity();

							// we grab the replytime which is for us
							uint16_t replyTime;
							memcpy(&replyTime, receivedData + SHORT_MAC_LEN + 2 + 2 + i * pollDeviceSize, 2);
							myDistantDevice->timePollReceived = timePollReceived;
							transmitPollAck(myDistantDevice, replyTime); // Acknowledge the POLL message

							noteActivity();

							if (_handleNewDevice != 0)
								(*_handleNewDevice)(myDistantDevice);

							return; // once we are done responding to POLL, we are done we dont need to loop to other devices
						}
					}
					// Even though we got a POLL message, we were not in the list. so we donot respond anything
				}
				else if (messageType == MessageType::RANGE)
				{
					DW1000Time timeRangeReceived;
					pDW1000.getReceiveTimestamp(timeRangeReceived);

					uint8_t numberDevices = receivedData[SHORT_MAC_LEN + 1];

					for (uint8_t i = 0; i < numberDevices; i++)
					{
						uint8_t shortAddress[2];
						memcpy(shortAddress, receivedData + SHORT_MAC_LEN + 2 + i * rangeDeviceSize, 2);

						// we test if the short address is our address
						if (shortAddress[0] == _ownShortAddress[0] &&
							shortAddress[1] == _ownShortAddress[1])
						{
							DW1000Device *myDistantDevice = searchDistantDevice(address);
							if (myDistantDevice != nullptr)
							{ // Cannot be a nullptr as we should have cached the TAG when we received the POLL

								myDistantDevice->noteActivity();

								// we grab the replytime which is for us
								myDistantDevice->timeRangeReceived = timeRangeReceived;

								myDistantDevice->setRXPower(pDW1000.getReceivePower());
								myDistantDevice->setFPPower(pDW1000.getFirstPathPower());
								myDistantDevice->setQuality(pDW1000.getReceiveQuality());

								myDistantDevice->timePollAckReceivedMinusPollSent.setTimestamp(receivedData + SHORT_MAC_LEN + 4 + rangeDeviceSize * i);
								myDistantDevice->timeRangeSentMinusPollAckReceived.setTimestamp(receivedData + SHORT_MAC_LEN + 9 + rangeDeviceSize * i);

								// (re-)compute range as two-way ranging is done
								DW1000Time myTOF;
								computeRangeAsymmetric(myDistantDevice, &myTOF); // CHOSEN RANGING ALGORITHM

								float distance = myTOF.getAsMeters();

								float payload;
								memcpy(&payload, receivedData + SHORT_MAC_LEN + 14 + rangeDeviceSize * i, 4);

								myDistantDevice->setPayload(payload);
								myDistantDevice->setRange(distance);

								noteActivity();

								if (ENABLE_RANGE_REPORT)
								{
									uint16_t replyTime = getReplyTimeOfIndex(i);

									// we send the range to TAG
									transmitRangeReport(myDistantDevice, replyTime);
								}

								// we have finished our range computation. We send the corresponding handler
								if (_handleNewRange != 0)
									(*_handleNewRange)(myDistantDevice);
							}
							else
							{
								// we received a range, towards us, but the device was missing from our list?
								//
								_portable.log_err(DW_RANGING, "RANGE received from TAG but, the TAG %x:%x not found in database", address[0], address[1]);
							}

							return;
						}
					}
				}
			}
			else if (_type == BoardType::TAG)
			{
				// if message was not for us, ignore?
				if (receivedData[6] != _ownShortAddress[0] || receivedData[5] != _ownShortAddress[1])
					return;

				if (messageType == MessageType::POLL_ACK) // POLL_ACK is a UNICAST message
				{
					DW1000Device *myDistantDevice = searchDistantDevice(address);

					if (myDistantDevice != nullptr)
					{
						pDW1000.getReceiveTimestamp(myDistantDevice->timePollAckReceived);
						myDistantDevice->noteActivity();
						myDistantDevice->hasSentPollAck = true;

						noteActivity();
						_portable.log_inf(DW_RANGING, "RANGE on POLL_ACK");

						transmitRange();
					}
					else
					{
						_portable.log_err(DW_RANGING, "POLL_ACK received, but the anchor %x:%x not found in database", address[0], address[1]);
					}
				}
				else if (messageType == MessageType::RANGE_REPORT) // TODO: Later
				{
					float curRange;
					memcpy(&curRange, receivedData + 1 + SHORT_MAC_LEN, 4);
					float curRXPower;
					memcpy(&curRXPower, receivedData + 5 + SHORT_MAC_LEN, 4);

					DW1000Device *myDistantDevice = searchDistantDevice(address);

					if (myDistantDevice != nullptr)
					{
						// we have a new range to save !
						myDistantDevice->setRange(curRange);
						myDistantDevice->setRXPower(curRXPower);

						// We can call our handler !
						// we have finished our range computation. We send the corresponding handler
						if (_handleNewRange != 0)
						{
							(*_handleNewRange)(myDistantDevice);
						}
					}
					else
					{
						_portable.log_err(DW_RANGING, "Device %x:%x not found in database", address[0], address[1]);
					}
				}
				else if (messageType == MessageType::RANGE_FAILED)
				{
					return;
				}
			}
		}
	}
}

/* ###########################################################################
 * #### Private methods and Handlers for transmit & Receive reply ############
 * ########################################################################### */

void DW1000Ranging::handleSent()
{
	// status change on sent success
	_sentAck = true;
}

void DW1000Ranging::handleReceived()
{
	// status change on received success
	_receivedAck = true;
	_portable.log_inf(DW_RANGING, "Received a frame....");
}

void DW1000Ranging::noteActivity()
{
	// update activity timestamp, so that we do not reach "resetPeriod"
	_lastActivity = _portable.millis();
}

void DW1000Ranging::resetInactive()
{
	// if inactive
	if (_portable.millis() - _lastActivity > _resetPeriod)
	{
		// VISHWA: reset to receive mode on reset_period
		// will this help? only anchor was getting reset
		// we shall do that for tag also?

		if (_type == BoardType::ANCHOR)
		{
			// receiver();
		}

		receiver();
		noteActivity();
	}
}

void DW1000Ranging::timerTick()
{
	if (counterForBlink == 0)
	{
		if (_type == BoardType::TAG)
		{
			transmitBlink();
		}
		// check for inactive devices if we are a TAG or ANCHOR
		checkForInactiveDevices();
	}
	else
	{
		if (_networkDevicesNumber > 0 && _type == BoardType::TAG)
		{
			// send a multicast poll
			transmitPoll();
		}
	}
	counterForBlink = (counterForBlink + 1) % BLINK_INTERVAL;
}

void DW1000Ranging::copyShortAddress(uint8_t to[], uint8_t from[])
{
	to[0] = from[0];
	to[1] = from[1];
}

/* ###########################################################################
 * #### Methods for ranging protocol   #######################################
 * ########################################################################### */

void DW1000Ranging::transmitInit()
{
	pDW1000.newTransmit();
	pDW1000.setDefaults();
}

void DW1000Ranging::transmit(uint8_t datas[])
{
	pDW1000.setData(datas, LEN_DATA);
	pDW1000.startTransmit();
}

void DW1000Ranging::transmit(uint8_t datas[], DW1000Time time)
{
	pDW1000.setDelay(time);
	pDW1000.setData(datas, LEN_DATA);
	pDW1000.startTransmit();
}

void DW1000Ranging::transmitBlink()
{
	// we need to set our timerDelay:
	_timerDelay = _rangeInterval + (uint16_t)(10 * 3 * DEFAULT_REPLY_DELAY_TIME / 1000);

	transmitInit();
	_globalMac.generateBlinkFrame(sentData, _ownShortAddress);

	sentData[BLINK_MAC_LEN] = _networkDevicesNumber;
	for (uint8_t i = 0; i < _networkDevicesNumber; i++)
	{
		memcpy(sentData + BLINK_MAC_LEN + 1 + i * 2, _networkDevices[i].getByteShortAddress(), 2);
	}
	transmit(sentData);

	uint8_t shortBroadcast[2] = {0xFF, 0xFF};
	copyShortAddress(_lastSentToShortAddress, shortBroadcast);
}

void DW1000Ranging::transmitRangingInit(DW1000Device *myDistantDevice, uint16_t delay)
{
	transmitInit();
	// we generate the mac frame for a ranging init message
	_globalMac.generateShortMACFrame(sentData, _ownShortAddress, myDistantDevice->getByteShortAddress());
	// we define the function code
	sentData[SHORT_MAC_LEN] = static_cast<uint8_t>(MessageType::RANGING_INIT);
	DW1000Time deltaTime = DW1000Time(delay, DW1000Time::MICROSECONDS);
	transmit(sentData, deltaTime);
}

void DW1000Ranging::transmitPoll()
{

	_portable.log_err(DW_RANGING, "Transmitting POLL");
	transmitInit();

	// we need to set our timerDelay:
	_timerDelay = _rangeInterval + (uint16_t)(pollAckTimeSlots * 3 * DEFAULT_REPLY_DELAY_TIME / 1000); // TODO meglio fermare il timer forse

	uint8_t devicesCount = _networkDevicesNumber < devicePerPollTransmit ? _networkDevicesNumber : devicePerPollTransmit;

	uint8_t shortBroadcast[2] = {0xFF, 0xFF};
	_globalMac.generateShortMACFrame(sentData, _ownShortAddress, shortBroadcast);
	sentData[SHORT_MAC_LEN] = static_cast<uint8_t>(MessageType::POLL);
	// we enter the number of devices
	sentData[SHORT_MAC_LEN + 1] = devicesCount;

	// uint8_t freeSlots = pollAckTimeSlots - devicesCount;

	for (uint8_t i = 0; i < devicesCount; i++)
	{
		// each devices have a different reply delay time.
		_networkDevices[i].setReplyTime(getReplyTimeOfIndex(i));

		// we write the short address of our device:
		memcpy(sentData + SHORT_MAC_LEN + 2 + i * pollDeviceSize, _networkDevices[i].getByteShortAddress(), 2);

		// we add the replyTime
		uint16_t replyTime = _networkDevices[i].getReplyTime();
		memcpy(sentData + SHORT_MAC_LEN + 2 + 2 + i * pollDeviceSize, &replyTime, 2);

		_replyTimeOfLastPollAck = replyTime;
	}

	_timeOfLastPollSent = _portable.millis();

	copyShortAddress(_lastSentToShortAddress, shortBroadcast);

	transmit(sentData);
}

void DW1000Ranging::transmitPollAck(DW1000Device *myDistantDevice, uint16_t delay)
{
	transmitInit();
	_globalMac.generateShortMACFrame(sentData, _ownShortAddress, myDistantDevice->getByteShortAddress());
	sentData[SHORT_MAC_LEN] = static_cast<uint8_t>(MessageType::POLL_ACK);
	// delay the same amount as ranging tag
	DW1000Time deltaTime = DW1000Time(delay, DW1000Time::MICROSECONDS);
	transmit(sentData, deltaTime);
}

void DW1000Ranging::transmitRange()
{
	// Disable range send on timeout
	_replyTimeOfLastPollAck = 0;
	_timeOfLastPollSent = 0;

	// _expectedMsgId = ENABLE_RANGE_REPORT ? MessageType::RANGE_REPORT : MessageType::POLL_ACK;

	constexpr uint8_t devicePerTransmit = 6;

	uint8_t devicesCount = 0;
	DW1000Device *devices[devicePerTransmit];
	for (uint8_t i = 0; i < _networkDevicesNumber && devicesCount < devicePerTransmit; i++)
	{
		if (_networkDevices[i].hasSentPollAck && !_networkDevices[i].hasRangeBeenServed)
		{
			devices[devicesCount++] = &_networkDevices[i];
		}
	}

	if (devicesCount == 0)
	{
		_portable.log_inf(DW_RANGING, "Not transmitting range as no anchor has sent any POLL_ACK yet");
		return;
	}
	// we need to set our timerDelay:
	_timerDelay = _rangeInterval + (uint16_t)(devicesCount * 3 * DEFAULT_REPLY_DELAY_TIME / 1000);

	transmitInit();

	uint8_t shortBroadcast[2] = {0xFF, 0xFF};
	_globalMac.generateShortMACFrame(sentData, _ownShortAddress, shortBroadcast);
	sentData[SHORT_MAC_LEN] = static_cast<uint8_t>(MessageType::RANGE);
	// we enter the number of devices
	sentData[SHORT_MAC_LEN + 1] = devicesCount;

	// delay sending the message and remember expected future sent timestamp
	DW1000Time deltaTime = DW1000Time(DEFAULT_REPLY_DELAY_TIME, DW1000Time::MICROSECONDS);
	DW1000Time timeRangeSent = pDW1000.setDelay(deltaTime);

	for (uint8_t i = 0; i < devicesCount; i++)
	{
		if (ENABLE_RANGE_REPORT)
			// each devices have a different reply delay time.
			devices[i]->setReplyTime(getReplyTimeOfIndex(i));

		// we get the device which correspond to the message which was sent (need to be filtered by MAC address)
		devices[i]->hasRangeBeenServed = true;

		devices[i]->timeRangeSent = timeRangeSent;
		devices[i]->timePollAckReceivedMinusPollSent = devices[i]->timePollAckReceived - devices[i]->timePollSent;
		devices[i]->timeRangeSentMinusPollAckReceived = devices[i]->timeRangeSent - devices[i]->timePollAckReceived;
		// we write the short address of our device:
		memcpy(sentData + SHORT_MAC_LEN + 2 + rangeDeviceSize * i, devices[i]->getByteShortAddress(), 2);
		devices[i]->timePollAckReceivedMinusPollSent.getTimestamp(sentData + SHORT_MAC_LEN + 4 + rangeDeviceSize * i);
		devices[i]->timeRangeSentMinusPollAckReceived.getTimestamp(sentData + SHORT_MAC_LEN + 9 + rangeDeviceSize * i);
		// we write the payload data
		memcpy(sentData + SHORT_MAC_LEN + 14 + rangeDeviceSize * i, &_payload, 4);
	}

	transmit(sentData);
}

void DW1000Ranging::transmitRangeReport(DW1000Device *myDistantDevice, uint16_t delay)
{
	transmitInit();
	_globalMac.generateShortMACFrame(sentData, _ownShortAddress, myDistantDevice->getByteShortAddress());
	sentData[SHORT_MAC_LEN] = static_cast<uint8_t>(MessageType::RANGE_REPORT);
	// write final ranging result
	float curRange = myDistantDevice->getRange();
	float curRXPower = myDistantDevice->getRXPower();
	// We add the Range and then the RXPower
	memcpy(sentData + 1 + SHORT_MAC_LEN, &curRange, 4);
	memcpy(sentData + 5 + SHORT_MAC_LEN, &curRXPower, 4);
	copyShortAddress(_lastSentToShortAddress, myDistantDevice->getByteShortAddress());
	transmit(sentData, DW1000Time(delay, DW1000Time::MICROSECONDS));
}

void DW1000Ranging::transmitRangeFailed(DW1000Device *myDistantDevice)
{
	transmitInit();
	_globalMac.generateShortMACFrame(sentData, _ownShortAddress, myDistantDevice->getByteShortAddress());
	sentData[SHORT_MAC_LEN] = static_cast<uint8_t>(MessageType::RANGE_FAILED);

	copyShortAddress(_lastSentToShortAddress, myDistantDevice->getByteShortAddress());
	transmit(sentData);
}

void DW1000Ranging::receiver()
{
	pDW1000.newReceive();
	pDW1000.setDefaults();
	// so we don't need to restart the receiver manually
	pDW1000.receivePermanently(true);
	pDW1000.startReceive();
}

uint16_t DW1000Ranging::getReplyTimeOfIndex(int i)
{
	return (2 * i + 1) * DEFAULT_REPLY_DELAY_TIME;
}

/* ###########################################################################
 * #### Methods for range computation and corrections  #######################
 * ########################################################################### */

void DW1000Ranging::computeRangeAsymmetric(DW1000Device *myDistantDevice, DW1000Time *myTOF)
{
	// asymmetric two-way ranging (more computation intense, less error prone)
	DW1000Time round1 = (myDistantDevice->timePollAckReceivedMinusPollSent).wrap();
	DW1000Time reply1 = (myDistantDevice->timePollAckSent - myDistantDevice->timePollReceived).wrap();
	DW1000Time round2 = (myDistantDevice->timeRangeReceived - myDistantDevice->timePollAckSent).wrap();
	DW1000Time reply2 = (myDistantDevice->timeRangeSentMinusPollAckReceived).wrap();

	myTOF->setTimestamp((round1 * round2 - reply1 * reply2) / (round1 + round2 + reply1 + reply2));

	/*
	_portable.log_vrb(DW_RANGING, LOG_DW1000_MSG, "timePollAckReceivedMinusPollSent %lld", myDistantDevice->timePollAckReceivedMinusPollSent.getTimestamp());
	_portable.log_vrb(DW_RANGING, LOG_DW1000_MSG, "round1 %lld", (long)round1.getTimestamp());

	_portable.log_vrb(DW_RANGING, LOG_DW1000_MSG, "timePollAckSent %lld", myDistantDevice->timePollAckSent.getTimestamp());
	_portable.log_vrb(DW_RANGING, LOG_DW1000_MSG, "timePollReceived %lld", myDistantDevice->timePollReceived.getTimestamp());
	_portable.log_vrb(DW_RANGING, LOG_DW1000_MSG, "reply1 %lld", (long)reply1.getTimestamp());

	_portable.log_vrb(DW_RANGING, LOG_DW1000_MSG, "timeRangeReceived %lld", myDistantDevice->timeRangeReceived.getTimestamp());
	_portable.log_vrb(DW_RANGING, LOG_DW1000_MSG, "timePollAckSent %lld", myDistantDevice->timePollAckSent.getTimestamp());
	_portable.log_vrb(DW_RANGING, LOG_DW1000_MSG, "round2 %lld", (long)round2.getTimestamp());

	_portable.log_vrb(DW_RANGING, LOG_DW1000_MSG, "timeRangeSentMinusPollAckReceived %lld", myDistantDevice->timeRangeSentMinusPollAckReceived.getTimestamp());
	_portable.log_vrb(DW_RANGING, LOG_DW1000_MSG, "reply2 %lld", (long)reply2.getTimestamp());
	*/
}
