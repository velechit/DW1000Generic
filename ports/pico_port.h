#ifndef __PICO_PORT_H__
#define __PICO_PORT_H__

#include "portables.h"

class PicoPort:public PortableCode {
    public:
        virtual void delay_ms(unsigned long) ;
        virtual void delay_us(unsigned int) ;

        virtual void initHW() ;
        virtual void deinitHW() ;

        virtual void asserReset(bool _high) ;
        virtual void deassertReset() ;

        virtual void spiChipSelect(bool select) ;
        virtual void spiBeginTransaction() ;
        virtual void spiEndTransaction() ;
        virtual uint8_t spiTransfer(uint8_t) ;
        virtual void spiSetSpeed(uint32_t speed) ;

        virtual void interruptCallbackCB(std::function<void()>)  ;

        virtual uint32_t millis() ;

        virtual void init() ;

        virtual long random(long min,long max)  ;
};

#endif