#include <iostream> // cout.rdbuf
#include "Logger.h"

Logger::Logger(Logger::Severity severity, bool useConsole, std::chrono::system_clock::time_point start_time)
	: minLevel(severity), outStream("Trace.log"), startTime(start_time)
{
	if (useConsole == true)
	{
		outStream.set_rdbuf(std::cout.rdbuf());
	}
}

Logger::~Logger()
{
	outStream.flush();
	outStream.close();
}

void Logger::LogError(std::string text)
{
	Log(Severity::Error, text);
}

void Logger::LogEvent(std::string text)
{
	Log(Severity::Event, text);
}

void Logger::LogDebug(std::string text)
{
	Log(Severity::Debug, text);
}

void Logger::LogVerbose(std::string text)
{
	Log(Severity::Verbose, text);
}


void Logger::Log(Logger::Severity severity, std::string message)
{
	if (severity >= minLevel)
	{
		switch (severity)
		{
		case Severity::Verbose:
			outStream.precision(4);
			outStream << '[' << std::fixed << GetSecondsSinceStart() << "]\t";
			outStream << "Verb" << '\t' << message << '\n';
			break;
		case Severity::Debug:
			outStream.precision(4);
			outStream << '[' << std::fixed << GetSecondsSinceStart() << "]\t";
			outStream << "Debug" << '\t' << message << '\n';
			break;
		case Severity::Event:
			outStream.precision(4);
			outStream << '[' << std::fixed << GetSecondsSinceStart() << "]\t";
			outStream << "Event" << '\t' << message << '\n';
			break;
		case Severity::Error:
			outStream.precision(4);
			outStream << '[' << std::fixed << GetSecondsSinceStart() << "]\t";
			outStream << "Error" << '\t' << message << '\n';
			break;
		}
	}
}

double Logger::GetSecondsSinceStart()
{
	return std::chrono::duration<double>(std::chrono::system_clock::now() - startTime).count();
}