#include "Logger.h"
#include "config.h" // for MAX_CLIENTS, SysSettings

Logger::LogLevel Logger::logLevel = Logger::Info;
uint32_t Logger::lastLogTime = 0;

/*
 * Output a debug message with a variable amount of parameters.
 * printf() style, see Logger::log()
 */
void Logger::debug(const char *message, ...)
{
    if (logLevel > Debug)
        return;
    va_list args;
    va_start(args, message);
    Logger::log(Debug, message, args);
    va_end(args);
}

/*
 * Output an info message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::info(const char *message, ...)
{
    if (logLevel > Info)
        return;
    va_list args;
    va_start(args, message);
    Logger::log(Info, message, args);
    va_end(args);
}

/*
 * Output a warning message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::warn(const char *message, ...)
{
    if (logLevel > Warn)
        return;
    va_list args;
    va_start(args, message);
    Logger::log(Warn, message, args);
    va_end(args);
}

/*
 * Output an error message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::error(const char *message, ...)
{
    if (logLevel > Error)
        return;
    va_list args;
    va_start(args, message);
    Logger::log(Error, message, args);
    va_end(args);
}

/*
 * Output a console message with a variable amount of parameters
 * printf() style, see Logger::logMessage()
 */
void Logger::console(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    Logger::logMessage(message, args);
    va_end(args);
}

/*
 * Set the log level. Any output below the specified log level will be omitted.
 */
void Logger::setLoglevel(LogLevel level)
{
    logLevel = level;
}

/*
 * Retrieve the current log level.
 */
Logger::LogLevel Logger::getLogLevel()
{
    return logLevel;
}

/*
 * Return a timestamp when the last log entry was made.
 */
uint32_t Logger::getLastLogTime()
{
    return lastLogTime;
}

/*
 * Returns if debug log level is enabled.
 */
boolean Logger::isDebug()
{
    return logLevel == Debug;
}

/*
 * Output a log message (called by debug(), info(), warn(), error(), console())
 *
 * Supports printf-like syntax:
 * %% %s %d %f %x %X %l %c %t %T
 */
void Logger::log(LogLevel level, const char *format, va_list args)
{
    lastLogTime = millis();
    Serial.print(lastLogTime);
    Serial.print(" - ");

    switch (level)
    {
    case Debug:
        Serial.print("DEBUG");
        break;
    case Info:
        Serial.print("INFO");
        break;
    case Warn:
        Serial.print("WARNING");
        break;
    case Error:
        Serial.print("ERROR");
        break;
    }

    Serial.print(": ");

    logMessage(format, args);
}

/*
 * Output a log message payload (printf-like)
 * Uses snprintf to avoid buffer overflow.
 * C++11-safe (no generic lambdas).
 */
void Logger::logMessage(const char *format, va_list args)
{
    uint8_t buffer[200];
    size_t buffLen = 0;

    // returns remaining space in buffer
    struct SpaceLeft
    {
        static size_t calc(size_t used, size_t total)
        {
            return (used < total) ? (total - used) : 0;
        }
    };

    const char *p = format;
    while (*p && SpaceLeft::calc(buffLen, sizeof(buffer)) > 0)
    {
        if (*p != '%')
        {
            buffer[buffLen++] = (uint8_t)*p++;
            continue;
        }

        // saw '%'
        ++p;
        if (!*p)
            break; // stray '%' at end

        char spec = *p++;
        size_t room = SpaceLeft::calc(buffLen, sizeof(buffer));
        int n = 0;

        switch (spec)
        {
        case '%':
            if (room == 0)
                goto flush;
            buffer[buffLen++] = '%';
            break;

        case 's':
        {
            char *s = va_arg(args, char *); // <- correct type
            if (!s)
                s = (char *)"";
            n = snprintf((char *)&buffer[buffLen], room, "%s", s);
            if (n <= 0)
                goto flush;
            if ((size_t)n >= room)
            {
                buffLen = sizeof(buffer);
                goto flush;
            }
            buffLen += (size_t)n;
            break;
        }

        case 'd':
        case 'i':
        {
            int v = va_arg(args, int);
            n = snprintf((char *)&buffer[buffLen], room, "%d", v);
            if (n <= 0)
                goto flush;
            if ((size_t)n >= room)
            {
                buffLen = sizeof(buffer);
                goto flush;
            }
            buffLen += (size_t)n;
            break;
        }

        case 'f':
        {
            double v = va_arg(args, double);
            n = snprintf((char *)&buffer[buffLen], room, "%.2f", v);
            if (n <= 0)
                goto flush;
            if ((size_t)n >= room)
            {
                buffLen = sizeof(buffer);
                goto flush;
            }
            buffLen += (size_t)n;
            break;
        }

        case 'x':
        { // hex (lowercase)
            int v = va_arg(args, int);
            n = snprintf((char *)&buffer[buffLen], room, "%x", v);
            if (n <= 0)
                goto flush;
            if ((size_t)n >= room)
            {
                buffLen = sizeof(buffer);
                goto flush;
            }
            buffLen += (size_t)n;
            break;
        }

        case 'X':
        { // hex with 0x prefix, uppercase
            int v = va_arg(args, int);
            n = snprintf((char *)&buffer[buffLen], room, "0x%X", v);
            if (n <= 0)
                goto flush;
            if ((size_t)n >= room)
            {
                buffLen = sizeof(buffer);
                goto flush;
            }
            buffLen += (size_t)n;
            break;
        }

        case 'l':
        { // long
            long v = va_arg(args, long);
            n = snprintf((char *)&buffer[buffLen], room, "%ld", v);
            if (n <= 0)
                goto flush;
            if ((size_t)n >= room)
            {
                buffLen = sizeof(buffer);
                goto flush;
            }
            buffLen += (size_t)n;
            break;
        }

        case 'c':
        {
            int v = va_arg(args, int);
            if (room == 0)
                goto flush;
            buffer[buffLen++] = (uint8_t)v;
            break;
        }

        case 't':
        { // boolean T/F
            int v = va_arg(args, int);
            if (room == 0)
                goto flush;
            buffer[buffLen++] = (v == 1) ? 'T' : 'F';
            break;
        }

        case 'T':
        { // boolean TRUE/FALSE
            int v = va_arg(args, int);
            const char *s = (v == 1) ? "TRUE" : "FALSE";
            n = snprintf((char *)&buffer[buffLen], room, "%s", s);
            if (n <= 0)
                goto flush;
            if ((size_t)n >= room)
            {
                buffLen = sizeof(buffer);
                goto flush;
            }
            buffLen += (size_t)n;
            break;
        }

        default:
            // Unknown specifier: emit it verbatim
            if (room >= 2)
            {
                buffer[buffLen++] = '%';
                buffer[buffLen++] = (uint8_t)spec;
            }
            else
            {
                goto flush;
            }
            break;
        }
    }

flush:
{
    size_t room = SpaceLeft::calc(buffLen, sizeof(buffer));
    if (room >= 2)
    {
        buffer[buffLen++] = '\r';
        buffer[buffLen++] = '\n';
    }
    else if (room >= 1)
    {
        buffer[buffLen++] = '\n';
    }
}

    Serial.write(buffer, buffLen);

    // If wifi has connected nodes then send to them too.
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (SysSettings.clientNodes[i] && SysSettings.clientNodes[i].connected())
        {
            SysSettings.clientNodes[i].write(buffer, buffLen);
        }
    }
}
