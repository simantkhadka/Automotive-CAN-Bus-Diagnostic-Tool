#ifndef LOGGER_H_
#define LOGGER_H_

#include <Arduino.h>
#include "config.h"

class Logger
{
public:
    enum LogLevel
    {
        Debug = 0,
        Info = 1,
        Warn = 2,
        Error = 3,
        Off = 4
    };
    static void debug(const char *, ...);
    static void info(const char *, ...);
    static void warn(const char *, ...);
    static void error(const char *, ...);
    static void console(const char *, ...);
    static void setLoglevel(LogLevel);
    static LogLevel getLogLevel();
    static uint32_t getLastLogTime();
    static boolean isDebug();
    static void loop();

private:
    static LogLevel logLevel;
    static uint32_t lastLogTime;

    static void log(LogLevel, const char *format, va_list);
    static void logMessage(const char *format, va_list args);
};

#endif
