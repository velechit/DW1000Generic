#ifndef __ARDUINO_PORT_H__
#define __ARDUINO_PORT_H__

#include "portables.h"

#include <Arduino.h>
#include <SPI.h>

class ArduinoPort:public PortableCode {
    protected:
        SPISettings _currentSPI;
        std::function<void()> interrupt_handler;
        SemaphoreHandle_t _sem;

        static ArduinoPort *_self;
        //portMAX_DELAY
        // 100 / portTICK_PERIOD_MS
        static void intr_task(void *);
        static void IRAM_ATTR handleInterrupt();

    public:
        ArduinoPort() { _self=this; init(); interrupt_handler=NULL; spiSetSpeed(2000000L);}
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