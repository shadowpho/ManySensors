#include "logging.h"
#include <string.h>
#include "stdio.h"
#include <cstdarg>

/*
void log_to_http(const char *format, ...)
{
    static char http_log_buffer[512];

    va_list args;
    va_start(args, format);
    int written = vsnprintf(http_log_buffer, sizeof(http_log_buffer), format, args);
    va_end(args);

    if (written < 0)
    {
        printf("Error formatting log message\n");
    }
    else
    {

        if ((size_t)written >= sizeof(http_log_buffer))
        {
            // Truncation
            written = sizeof(http_log_buffer) - 1;
        }
#if !defined(ENABLE_LIGHT_SLEEP) || defined(ENABLE_DEBUG_UART)
        printf("%s", http_log_buffer);
#endif

        start_wifi(), http_send_logs(written, http_log_buffer), stop_wifi();
    }
}
    */