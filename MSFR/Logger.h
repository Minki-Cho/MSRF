#pragma once
#include <string>
#include <fstream>
#include <chrono>
#include <mutex>

#include <Windows.h>

class Logger
{
public:
    enum class Severity
    {
        Verbose,
        Debug,
        Event,
        Error,
    };

    Logger();
    Logger(Severity severity, bool useConsole, std::chrono::system_clock::time_point start_time);
    ~Logger();

    void LogError(const std::string& text);
    void LogEvent(const std::string& text);
    void LogDebug(const std::string& text);
    void LogVerbose(const std::string& text);

    void SetUseConsole(bool enabled) noexcept;
    bool IsUsingConsole() const noexcept;
    void SetFocusRestoreHwnd(HWND hwnd) noexcept;
private:
    void Log(Severity severity, const std::string& displayText);
    double GetSecondsSinceStart() const;

private:
    std::ofstream fileStream;
    Severity minLevel;
    std::chrono::system_clock::time_point startTime;

    bool useConsole = false;
    HWND focusRestoreHwnd = nullptr;
    mutable std::mutex mtx;

};
