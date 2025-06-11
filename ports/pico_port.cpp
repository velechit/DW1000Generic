#include "pico_port.h"


void PicoPort::delay_ms(unsigned long ms) {
    // delay 1 ms
}

void PicoPort::delay_us(unsigned int us) {
    // delay 1us
}
void PicoPort::init(){
   // initialize millis
}

void PicoPort::asserReset(bool _high){
    // make reset pin as output 
    // and pull it HIGH if _high is true LOW if _high is false
}
void PicoPort::deassertReset(){
    // float the reset pin
}

void PicoPort::initHW(){
    // setup all the GPIOs
    // keep reset pin input
    // Initialize SPI
    //      2000000L, MSBFIRST, SPI_MODE0
}
void PicoPort::deinitHW(){
    // stop SPI, make all GPIO's into some state
}

void PicoPort::spiChipSelect(bool select){
    // put chipselect of the DWM to LOW if select is true
    // put chipselect of the DWM to HIGH if select is false
}
void PicoPort::spiSetSpeed(uint32_t speed){
    // set the SPI HW speed to speed, restart SPI if needed
    //    speed, MSBFIRST, SPI_MODE0
}

void PicoPort::spiBeginTransaction(){
    // start SPI transaction
}
void PicoPort::spiEndTransaction(){
    // end SPI transaction
}
uint8_t PicoPort::spiTransfer(uint8_t dataIn){ 
    return dataIn;
} // implement properly

void PicoPort::interruptCallbackCB(std::function<void()> callable){
    // setup interrupt on IRQ pin and call the "callable" on interrupt
}

uint32_t PicoPort::millis(){ // Implement a millis
    return 0;
}

long PicoPort::random(long min,long max){
    // return a random number between min and max
   return 0;
}
