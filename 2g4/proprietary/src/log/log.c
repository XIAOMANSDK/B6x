#include "log.h"
#include <stdio.h>
#include <stdarg.h>

#define LOG_LINE_LENGTH     ( 64 )
#define LOG_MAX_HEX         ( 32 )
#define LOG_HEX_LENGTH      ( LOG_MAX_HEX * 3 + 3 )

int log_printf(logout_fn logout, log_level_t level, log_level_t current, const char *format, ...)
{
    char line[LOG_LINE_LENGTH + 3] = "L: ";
    char type[LOG_LEVEL_ALL + 2] = "EWIDL";
    va_list valist;
    int r;

    if (current > level || current == LOG_LEVEL_NONE)
    {
        return 0;
    }
    line[0] = type[current - 1];

    va_start(valist, format);

    r = vsnprintf(line + 3, LOG_LINE_LENGTH , format, valist);

    va_end(valist);

    if (r > 0)
    {
        logout(line);
    }

    return r;
}

int log_hex(logout_fn logout, log_level_t level, log_level_t current, uint8_t *data, uint16_t length)
{
    char *hex = "0123456789ABCDEF";
    char line[LOG_HEX_LENGTH];
    if (current > level || current == LOG_LEVEL_NONE)
    {
        return 0;
    }
    if (length > LOG_MAX_HEX)
    {
        length = LOG_MAX_HEX;
    }
    char *p = line;
    while (length--)
    {
        *p++ = hex[(*data >> 4) & 0x0F];
        *p++ = hex[*data & 0x0F];
        *p++ = ' ';
        data++;
    }
    *p++ = '\r';
    *p++ = '\n';
    *p++ = 0;
    logout(line);

    return (p - line);
}
