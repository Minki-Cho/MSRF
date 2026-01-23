#pragma once
#include <string>
#include <fstream> //file output
#include <chrono> //time

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

private:
	void Log(Severity, std::string displayText);
	double GetSecondsSinceStart();

	std::ofstream outStream;
	Severity minLevel;
	std::chrono::system_clock::time_point startTime;
};