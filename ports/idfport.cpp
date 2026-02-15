#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "idfport.h"
#include "rom/ets_sys.h"

#include "esp_timer.h"
#include "esp_random.h"

#define PIN_LED GPIO_NUM_1

#define PIN_RST GPIO_NUM_39
#define PIN_IRQ GPIO_NUM_40
#define PIN_SS GPIO_NUM_10

#define SPI_MOSI 11
#define SPI_MISO 13
#define SPI_CLK 12

IDFPort *IDFPort::_self;

void IRAM_ATTR IDFPort::_gpio_isr_helper(void *arg)
{
    if (_self->_sem)
        xSemaphoreGiveFromISR(_self->_sem, NULL);
}

void IDFPort::delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
void IDFPort::delay_us(uint32_t us)
{
    ets_delay_us(us);
}

uint32_t IDFPort::millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

int IDFPort::random(int min, int max)
{
    return (esp_random() % (max - min)) + min;
}

void IDFPort::dw1000_reset(void)
{
    gpio_set_direction(PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RST, 0);
    delay_ms(2);
    gpio_set_direction(PIN_RST, GPIO_MODE_INPUT); // DW1000 Reset should not be pulled HIGH. it should be kept floating
    delay_ms(10);
}

void IDFPort::dw1000_select(bool _select)
{
    gpio_set_level(PIN_SS, _select ? 0 : 1);
}

void IDFPort::dw1000_spi_read(uint8_t *header, size_t hLen, uint8_t *data, size_t dLen)
{
    spi_transfer(header, hLen, data, dLen, true);
}
void IDFPort::dw1000_spi_write(uint8_t *header, size_t hLen, uint8_t *data, size_t dLen)
{
    spi_transfer(header, hLen, data, dLen, false);
}
void IDFPort::spi_transfer(uint8_t *header, size_t hLen, uint8_t *data, size_t dLen, bool isRead)
{
    size_t total_len = hLen + dLen;
    if (total_len == 0)
        return;

    // 1. Allocate a single DMA-safe buffer for both TX and RX
    // We use MALLOC_CAP_DMA to ensure it works with the SPI peripheral
    uint8_t *combined_buf = (uint8_t *)heap_caps_malloc(total_len, MALLOC_CAP_DMA);
    if (combined_buf == NULL)
        return;

    // 2. Prepare the TX portion
    // Copy header to the start
    memset(combined_buf, 0, total_len);

    memcpy(combined_buf, header, hLen);
    if (!isRead && data && dLen > 0)
    {
        memcpy(combined_buf + hLen, data, dLen);
    }

    // 3. Set up the transaction
    spi_transaction_t t = {};
    t.length = total_len * 8;   // Total length in bits
    t.tx_buffer = combined_buf; // Send the whole thing
    t.rx_buffer = combined_buf; // Receive back into the SAME buffer (Full Duplex)

    // 4. Perform the transfer
    // Using polling for lower latency on small-to-medium transfers
    dw1000_select(true);
    spi_device_polling_transmit(_spi_handle, &t);
    delay_us(5);
    dw1000_select(false);

    // 5. If it was a Read, copy the received data back to the user's buffer
    // The received data starts exactly after the transmitted header bytes
    if (isRead && data && dLen > 0)
    {
        memcpy(data, combined_buf + hLen, dLen);
    }

    // 6. Cleanup
    free(combined_buf);
}

void IDFPort::begin()
{
    // CS, RST and IRQ gpios to be initialized
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_SS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    // Apply settings
    gpio_config(&io_conf);
    gpio_set_level(PIN_SS, 1);

    io_conf.pin_bit_mask = (1ULL << PIN_LED);
    gpio_config(&io_conf);
    gpio_set_level(PIN_LED, 1);

    io_conf.pin_bit_mask = (1ULL << PIN_RST);
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << PIN_IRQ);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(PIN_IRQ, IDFPort::_gpio_isr_helper, (void *)this);

    spi_bus_config_t buscfg;
    memset(&buscfg, 0, sizeof(spi_bus_config_t));
    buscfg.miso_io_num = SPI_MISO;
    buscfg.mosi_io_num = SPI_MOSI;
    buscfg.sclk_io_num = SPI_CLK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 4096;

    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg;
    memset(&devcfg, 0, sizeof(spi_device_interface_config_t));

    devcfg.clock_speed_hz = 16 * 1000 * 1000;
    devcfg.mode = 0;
    devcfg.spics_io_num = -1;
    devcfg.queue_size = 7;

    spi_bus_add_device(SPI2_HOST, &devcfg, &_spi_handle);
}

void IDFPort::dw1000_set_spi_speed(dw1000_spi_speed_t _speed)
{
    spi_device_interface_config_t devcfg;
    memset(&devcfg, 0, sizeof(spi_device_interface_config_t));
    devcfg.clock_speed_hz = ((_speed == FAST_SPI) ? 16 : 2) * 1000 * 1000;
    devcfg.mode = 0;
    devcfg.spics_io_num = -1;
    devcfg.queue_size = 7;

    spi_bus_remove_device(_spi_handle);
    spi_bus_add_device(SPI2_HOST, &devcfg, &_spi_handle);
}

void IDFPort::log(std::string const &tag, LogLevel level, const char *msg, std::va_list args)
{
    LogLevel minLevel = logLevels.count(tag) ? logLevels[tag] : _default_level;
    if (level >= minLevel)
    {

        if (_print_tag)
            printf("***%s : %s - ", _msg_type_pretty[level].c_str(), tag.c_str());
        else
            printf("***%s : ", _msg_type_pretty[level].c_str());

        vprintf(msg, args);
        printf("\n");
        va_end(args);
    }
}
void IDFPort::log_set_level(const std::string &_tag, const LogLevel _level)
{
    logLevels[_tag] = _level;
}

void IDFPort::log_err(const std::string &_tag, const char *msg, ...)
{
    std::va_list args;
    va_start(args, msg);
    log(_tag, LogLevel::LOG_LEVEL_ERROR, msg, args);
}
void IDFPort::log_war(const std::string &_tag, const char *msg, ...)
{
    std::va_list args;
    va_start(args, msg);
    log(_tag, LogLevel::LOG_LEVEL_WARNING, msg, args);
}
void IDFPort::log_inf(const std::string &_tag, const char *msg, ...)
{
    std::va_list args;
    va_start(args, msg);
    log(_tag, LogLevel::LOG_LEVEL_INFO, msg, args);
}
void IDFPort::log_dbg(const std::string &_tag, const char *msg, ...)
{
    std::va_list args;
    va_start(args, msg);
    log(_tag, LogLevel::LOG_LEVEL_DEBUG, msg, args);
}
void IDFPort::log_vrb(const std::string &_tag, const char *msg, ...)
{
    std::va_list args;
    va_start(args, msg);
    log(_tag, LogLevel::LOG_LEVEL_VERBOSE, msg, args);
}

void IDFPort::dw1000_irq_isr(std::function<void()> callable)
{
    interrupt_handler = callable;
    xTaskCreatePinnedToCore(IDFPort::_InvokeInterrupt, "InterruptTASK", 2048, NULL, 5, NULL, 0);
    // TODO: set up a RTOS TASK which will call callable() when interrupt semaphore is given
}

void IDFPort::_InvokeInterrupt(void *param)
{
    while (true)
    {
        if (xSemaphoreTake(_self->_sem, portMAX_DELAY) == pdTRUE)
        {
            if (_self->interrupt_handler != NULL)
                _self->interrupt_handler();
        }
    }
}