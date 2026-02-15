#pragma once

#include "portable.h"

#include <string.h>
#include <cstdarg>
#include <unordered_map>
#include <functional>

#include "driver/spi_master.h"

class IDFPort : public PortableCode
{
public:
    typedef enum
    {
        LOG_LEVEL_VERBOSE = 0,
        LOG_LEVEL_DEBUG = 1,
        LOG_LEVEL_INFO = 2,
        LOG_LEVEL_WARNING = 3,
        LOG_LEVEL_ERROR = 4,
    } LogLevel;

    IDFPort()
    {
        _self = this;
        _sem = xSemaphoreCreateBinary();
        _default_level = LogLevel::LOG_LEVEL_INFO;
        _print_tag = false;
    }
    uint32_t millis();
    void delay_ms(uint32_t);
    void delay_us(uint32_t);
    int random(int, int);
    void dw1000_reset(void);
    void dw1000_select(bool);
    void dw1000_spi_read(uint8_t *header, size_t hLen, uint8_t *data, size_t dLen);
    void dw1000_spi_write(uint8_t *header, size_t hLen, uint8_t *data, size_t dLen);
    void begin();
    void dw1000_set_spi_speed(dw1000_spi_speed_t);

    void dw1000_irq_isr(std::function<void()>);

    void log_enable_tag_print(bool enable) { _print_tag = enable; }
    void log_set_level(const std::string &, const LogLevel);
    void log_err(const std::string &tag, const char *msg, ...);
    void log_war(const std::string &tag, const char *msg, ...);
    void log_inf(const std::string &tag, const char *msg, ...);
    void log_dbg(const std::string &tag, const char *msg, ...);
    void log_vrb(const std::string &tag, const char *msg, ...);

protected:
    inline static std::unordered_map<LogLevel, std::string> _msg_type_pretty = {
        {LogLevel::LOG_LEVEL_VERBOSE, "Verbose"},
        {LogLevel::LOG_LEVEL_DEBUG, "Debug"},
        {LogLevel::LOG_LEVEL_INFO, "Info"},
        {LogLevel::LOG_LEVEL_WARNING, "Warning"},
        {LogLevel::LOG_LEVEL_ERROR, "Error"},
    };

    static IDFPort *_self;
    bool _print_tag;
    LogLevel _default_level;
    std::unordered_map<std::string, LogLevel> logLevels;
    void log(std::string const &tag, LogLevel level, const char *msg, std::va_list args);
    std::function<void()> interrupt_handler;
    spi_device_handle_t _spi_handle;
    static void IRAM_ATTR _gpio_isr_helper(void *arg);
    static void _InvokeInterrupt(void *);
    SemaphoreHandle_t _sem;
    void spi_transfer(uint8_t *header, size_t hLen, uint8_t *data, size_t dLen, bool isRead);
};
