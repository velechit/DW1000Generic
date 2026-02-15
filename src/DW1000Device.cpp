#include "DW1000Device.h"
#include "DW1000.h"

// Constructor and destructor
DW1000Device::DW1000Device(PortableCode &portable) : _portable(portable)
{
	noteActivity();
}
DW1000Device::DW1000Device(PortableCode &portable, uint8_t shortAddress[]) : _portable(portable)
{
	// we set the 2 bytes address
	setShortAddress(shortAddress);
	noteActivity();
}

DW1000Device::~DW1000Device() {}

// setters:
void DW1000Device::setShortAddress(uint8_t deviceAddress[])
{
	memcpy(_shortAddress, deviceAddress, 2);
}

uint16_t DW1000Device::getShortAddress()
{
	return _shortAddress[1] * 256 + _shortAddress[0];
}

bool DW1000Device::isShortAddressEqual(DW1000Device *device)
{
	return memcmp(this->getByteShortAddress(), device->getByteShortAddress(), 2) == 0;
}

void DW1000Device::noteActivity()
{
	_activity = _portable.millis();
}

bool DW1000Device::isInactive()
{
	// One second of inactivity
	return _portable.millis() - _activity > INACTIVITY_TIME;
}
