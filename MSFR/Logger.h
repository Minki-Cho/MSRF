#pragma once
#include <string>
#include <fstream>
#include <chrono>

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

    void LogError(std::string text);
    void LogEvent(std::string text);
    void LogDebug(std::string text);
    void LogVerbose(std::string text);

    void SetUseConsole(bool enable);
    bool IsUsingConsole() const { return usingConsole; }

private:
    void Log(Severity, std::string displayText);
    double GetSecondsSinceStart();

    std::ofstream outStream;
    std::streambuf* fileBuf = nullptr;
    bool usingConsole = false;

    Severity minLevel;
    std::chrono::system_clock::time_point startTime;
};
