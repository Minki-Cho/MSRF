#include "Logger.h"
#include "PlatformLogger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>

namespace
{
    const char* ToTag(Logger::Severity severity)
    {
        switch (severity)
        {
        case Logger::Severity::Verbose: return "Verb";
        case Logger::Severity::Debug:   return "Debug";
        case Logger::Severity::Event:   return "Event";
        case Logger::Severity::Warning: return "Warn";
        case Logger::Severity::Error:   return "Error";
        case Logger::Severity::Fatal:   return "Fatal";
        default:                        return "Unknown";
        }
    }
}

Logger::Logger()
    : Logger(Severity::Debug, true, std::chrono::system_clock::now())
{
}

Logger::Logger(Logger::Severity severity, bool useConsole_, std::chrono::system_clock::time_point start_time)
    : fileStream("Trace.log", std::ios::out | std::ios::trunc),
    minLevel(severity),
    startTime(start_time),
    useConsole(useConsole_)
{
    if (!fileStream.is_open())
    {
        platform::DebugOutput("[Logger] Failed to open Trace.log\n");
    }
}

Logger::~Logger()
{
    std::lock_guard<std::mutex> lock(mtx);
    if (fileStream.is_open())
    {
        fileStream.flush();
        fileStream.close();
    }
}

void Logger::SetUseConsole(bool enabled) noexcept
{
    useConsole = enabled;
}

bool Logger::IsUsingConsole() const noexcept
{
    return useConsole;
}

void Logger::SetFocusRestoreHwnd(void* nativeWindowHandle) noexcept
{
    focusRestoreHwnd = nativeWindowHandle;
}

void Logger::LogError(const std::string& text) { Log(Severity::Error, text); }
void Logger::LogEvent(const std::string& text) { Log(Severity::Event, text); }
void Logger::LogDebug(const std::string& text) { Log(Severity::Debug, text); }
void Logger::LogVerbose(const std::string& text) { Log(Severity::Verbose, text); }
void Logger::LogWarning(const std::string& text) { Log(Severity::Warning, text); }
void Logger::LogFatal(const std::string& text) { Log(Severity::Fatal, text); }

void Logger::LogWithContext(Severity severity, const std::string& message, const Context& context)
{
    std::ostringstream oss;
    oss << '[' << context.subsystem << "] " << message;
    oss << " | thread=" << std::this_thread::get_id();

    if (context.state && context.state[0] != '\0')
    {
        oss << '\n' << " | state=" << context.state;
    }
    if (context.code != 0)
    {
        oss << " | code=0x" << std::hex << std::uppercase << static_cast<unsigned long>(context.code) << std::dec;
    }

    if (context.file && context.file[0] != '\0')
    {
        oss << " | at " << context.file;
        if (context.line > 0)
        {
            oss << ':' << context.line;
        }

        if (context.function && context.function[0] != '\0')
        {
            oss << " (" << context.function << ')';
        }
    }

    Log(severity, oss.str());
}

void Logger::Log(Logger::Severity severity, const std::string& message)
{
    if (severity < minLevel) return;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << '[' << GetSecondsSinceStart() << "]\t"
        << ToTag(severity) << '\t' << message << '\n';
    const std::string line = oss.str();

    std::lock_guard<std::mutex> lock(mtx);

    if (fileStream.is_open())
    {
        fileStream << line;
        fileStream.flush();
    }

    platform::DebugOutput(line.c_str());

    if (useConsole)
    {
        std::cout << line;
        std::cout.flush();

        if (focusRestoreHwnd)
        {
            platform::RestoreWindowFocus(focusRestoreHwnd);
        }
    }
}

double Logger::GetSecondsSinceStart() const
{
    return std::chrono::duration<double>(std::chrono::system_clock::now() - startTime).count();
}
