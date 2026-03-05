#pragma once
#include <chrono>
#include <fstream>
#include <mutex>
#include <string>

class Logger
{
public:
    enum class Severity
    {
        Verbose,
        Debug,
        Event,
        Warning,
        Error,
        Fatal,
    };
    struct Context
    {
        const char* subsystem = "General";
        const char* file = "";
        const char* function = "";
        int line = 0;
        long code = 0;
        const char* state = nullptr;
    };

    Logger();
    Logger(Severity severity, bool useConsole, std::chrono::system_clock::time_point start_time);
    ~Logger();

    void LogError(const std::string& text);
    void LogEvent(const std::string& text);
    void LogDebug(const std::string& text);
    void LogVerbose(const std::string& text);
    void LogWarning(const std::string& text);
    void LogFatal(const std::string& text);

    void LogWithContext(Severity severity, const std::string& message, const Context& context);

    void SetUseConsole(bool enabled) noexcept;
    bool IsUsingConsole() const noexcept;
    void SetFocusRestoreHwnd(void* nativeWindowHandle) noexcept;

private:
    void Log(Severity severity, const std::string& displayText);
    double GetSecondsSinceStart() const;

private:
    std::ofstream fileStream;
    Severity minLevel;
    std::chrono::system_clock::time_point startTime;

    bool useConsole = false;
    void* focusRestoreHwnd = nullptr;
    mutable std::mutex mtx;
};

#define ENGINE_LOG_CTX(logger, severity, subsystemName, messageText) \
    (logger).LogWithContext((severity), (messageText), Logger::Context{ (subsystemName), __FILE__, __FUNCTION__, __LINE__, 0, nullptr })

#define ENGINE_LOG_HRESULT(logger, severity, subsystemName, messageText, hrCode) \
    (logger).LogWithContext((severity), (messageText), Logger::Context{ (subsystemName), __FILE__, __FUNCTION__, __LINE__, static_cast<long>(hrCode), nullptr })
