#pragma once

#include <stdint.h>
#include <string>
#include <functional>

class PortableCode
{
public:
	typedef enum
	{
		FAST_SPI = 0,
		SLOW_SPI
	} dw1000_spi_speed_t;

	// Delays
	virtual void delay_ms(uint32_t) = 0;
	virtual void delay_us(uint32_t) = 0;
	virtual uint32_t millis() = 0;

	virtual void begin() = 0;

	virtual void dw1000_reset(void) = 0;
	virtual void dw1000_select(bool) = 0;
	virtual void dw1000_irq_isr(std::function<void()>) = 0;
	virtual void dw1000_spi_read(uint8_t *header, size_t hLen, uint8_t *data, size_t dLen) = 0;
	virtual void dw1000_spi_write(uint8_t *header, size_t hLen, uint8_t *data, size_t dLen) = 0;
	virtual void dw1000_set_spi_speed(dw1000_spi_speed_t) = 0;

	virtual int random(int, int) = 0;

	virtual void log_err(const std::string &tag, const char *msg, ...) = 0;
	virtual void log_war(const std::string &tag, const char *msg, ...) = 0;
	virtual void log_inf(const std::string &tag, const char *msg, ...) = 0;
	virtual void log_dbg(const std::string &tag, const char *msg, ...) = 0;
	virtual void log_vrb(const std::string &tag, const char *msg, ...) = 0;
};
