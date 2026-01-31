#include <iostream>
#include "Logger.h"

#define NOMINMAX
#include <Windows.h>
#include <cstdio>

static void EnsureConsole()
{
    static bool created = false;
    if (created) return;

    AllocConsole();

    FILE* fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    std::ios::sync_with_stdio(true);
    created = true;
}

Logger::Logger()
    : Logger(Severity::Event, false, std::chrono::system_clock::now())
{
}

Logger::Logger(Logger::Severity severity, bool useConsole, std::chrono::system_clock::time_point start_time)
    : minLevel(severity), outStream("Trace.log"), startTime(start_time)
{
    fileBuf = outStream.rdbuf();
    SetUseConsole(useConsole);
}

Logger::~Logger()
{
    outStream.flush();
    outStream.close();
}

void Logger::SetUseConsole(bool enable)
{
    if (enable)
    {
        EnsureConsole();
        outStream.set_rdbuf(std::cout.rdbuf());
        usingConsole = true;
    }
    else
    {
        if (fileBuf)
            outStream.set_rdbuf(fileBuf);
        usingConsole = false;
    }
}

void Logger::LogError(std::string text) { Log(Severity::Error, text); }
void Logger::LogEvent(std::string text) { Log(Severity::Event, text); }
void Logger::LogDebug(std::string text) { Log(Severity::Debug, text); }
void Logger::LogVerbose(std::string text) { Log(Severity::Verbose, text); }

void Logger::Log(Logger::Severity severity, std::string message)
{
    if (severity >= minLevel)
    {
        outStream.precision(4);
        outStream << '[' << std::fixed << GetSecondsSinceStart() << "]\t";

        switch (severity)
        {
        case Severity::Verbose: outStream << "Verb";  break;
        case Severity::Debug:   outStream << "Debug"; break;
        case Severity::Event:   outStream << "Event"; break;
        case Severity::Error:   outStream << "Error"; break;
        }

        outStream << '\t' << message << '\n';
    }
}

double Logger::GetSecondsSinceStart()
{
    return std::chrono::duration<double>(std::chrono::system_clock::now() - startTime).count();
}
