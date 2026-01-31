#include "Logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#define NOMINMAX
#include <Windows.h> // OutputDebugStringA

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
        OutputDebugStringA("[Logger] Failed to open Trace.log\n");
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

void Logger::SetFocusRestoreHwnd(HWND hwnd) noexcept
{
    focusRestoreHwnd = hwnd;
}

void Logger::LogError(const std::string& text) { Log(Severity::Error, text); }
void Logger::LogEvent(const std::string& text) { Log(Severity::Event, text); }
void Logger::LogDebug(const std::string& text) { Log(Severity::Debug, text); }
void Logger::LogVerbose(const std::string& text) { Log(Severity::Verbose, text); }

void Logger::Log(Logger::Severity severity, const std::string& message)
{
    if (severity < minLevel) return;

    const char* tag = "????";
    switch (severity)
    {
    case Severity::Verbose: tag = "Verb";  break;
    case Severity::Debug:   tag = "Debug"; break;
    case Severity::Event:   tag = "Event"; break;
    case Severity::Error:   tag = "Error"; break;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << '[' << GetSecondsSinceStart() << "]\t"
        << tag << '\t' << message << '\n';

    const std::string line = oss.str();

    std::lock_guard<std::mutex> lock(mtx);

    if (fileStream.is_open())
    {
        fileStream << line;
        fileStream.flush();
    }

    OutputDebugStringA(line.c_str());

    if (useConsole)
    {
        std::cout << line;
        std::cout.flush();

        if (focusRestoreHwnd)
        {
            SetForegroundWindow(focusRestoreHwnd);
            SetFocus(focusRestoreHwnd);
        }
    }
}

double Logger::GetSecondsSinceStart() const
{
    return std::chrono::duration<double>(std::chrono::system_clock::now() - startTime).count();
}