#include "hw_conf.h"
#include "arduino_port.h"
#include "m_log.h"


ArduinoPort *ArduinoPort::_self;

void IRAM_ATTR ArduinoPort::handleInterrupt() { 
    if (_self->_sem) {   xSemaphoreGiveFromISR(_self->_sem, NULL);  } 
}

void ArduinoPort::intr_task(void *parameter) {
     while(1) {
         if(xSemaphoreTake(_self->_sem, portMAX_DELAY) == pdTRUE) {
             if(_self->interrupt_handler != NULL) 
                _self->interrupt_handler(); 
         }
     }
}

void ArduinoPort::delay_ms(unsigned long ms) {
    // delay 1 ms
    delay(ms);
}

void ArduinoPort::delay_us(unsigned int us) {
    // delay 1us
    delayMicroseconds(us);
}
void ArduinoPort::init(){
   // initialize millis
   // On Arduino no need to init millis() ... its already initialized
    _sem = xSemaphoreCreateBinary(); // Create the semaphore

}
void ArduinoPort::asserReset(bool _high){
    // make reset pin as output 
    // and pull it HIGH if _high is true LOW if _high is false
    pinMode(PIN_RST, OUTPUT);
    digitalWrite(PIN_RST, _high ? HIGH : LOW );

}
void ArduinoPort::deassertReset(){
    // float the reset pin
    pinMode(PIN_RST, INPUT);
    digitalWrite(PIN_RST, LOW );
}

void ArduinoPort::initHW(){
    // setup all the GPIOs
    // keep reset pin input
    // Initialize SPI
    //      2000000L, MSBFIRST, SPI_MODE0
    m_log::log_dbg(LOG_DW1000, "Initializing HW...");
    SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI);
    
    pinMode(PIN_RST, INPUT);
    pinMode(PIN_IRQ, INPUT);
    pinMode(PIN_SS, OUTPUT);
    digitalWrite(PIN_SS, HIGH);
}
void ArduinoPort::deinitHW(){
    // stop SPI, make all GPIO's into some state
    SPI.end();
}

void ArduinoPort::spiChipSelect(bool select){
    // put chipselect of the DWM to LOW if select is true
    // put chipselect of the DWM to HIGH if select is false
    digitalWrite(PIN_SS, select?LOW:HIGH);
}
void ArduinoPort::spiSetSpeed(uint32_t speed){
    // set the SPI HW speed to speed, restart SPI if needed
    //    speed, MSBFIRST, SPI_MODE0
    _currentSPI=SPISettings(speed, MSBFIRST, SPI_MODE0);
}

void ArduinoPort::spiBeginTransaction(){
    // start SPI transaction
    SPI.beginTransaction(_currentSPI);
}
void ArduinoPort::spiEndTransaction(){
    // end SPI transaction
    SPI.endTransaction();
}
uint8_t ArduinoPort::spiTransfer(uint8_t dataIn){ 
    return SPI.transfer(dataIn);
} // implement properly

void ArduinoPort::interruptCallbackCB(std::function<void()> callable){
    // setup interrupt on IRQ pin to call the "callable" on interrupt
    // attach interrupt to a pin to call this callable
    m_log::log_dbg(LOG_DW1000, "Attaching interrupt...");
    interrupt_handler = callable;
    xTaskCreatePinnedToCore(ArduinoPort::intr_task, "InterruptTASK", 2048, NULL, 5, NULL, 0);
   	attachInterrupt(digitalPinToInterrupt(PIN_IRQ), ArduinoPort::handleInterrupt, RISING);
}

uint32_t ArduinoPort::millis(){ // Implement a millis
    return ::millis();
}

long ArduinoPort::random(long min,long max){
    // return a random number between min and max
   return random(min,max);
}
