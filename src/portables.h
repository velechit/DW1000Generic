#ifndef __PORTABLES_H__
#define __PORTABLES_H__

#include <inttypes.h>
#include <functional>

class PortableCode {
    public:
        virtual void delay_ms(unsigned long) = 0;
        virtual void delay_us(unsigned int) = 0;

        virtual void initHW() = 0;
        virtual void deinitHW() = 0;

        virtual void asserReset(bool _high) = 0;
        virtual void deassertReset() = 0;

        virtual void spiChipSelect(bool select) = 0;
        virtual void spiBeginTransaction() = 0;
        virtual void spiEndTransaction() = 0;
        virtual uint8_t spiTransfer(uint8_t) = 0;
        virtual void spiSetSpeed(uint32_t speed) = 0;

        virtual void interruptCallbackCB(std::function<void()>)  = 0;

        virtual uint32_t millis() = 0;

        virtual void init() = 0;

        virtual long random(long min,long max)  = 0;
};

#endif /*! __PORTABLES_H__ */