//
// Debug.cpp
//

#include "pch.h"
#include "Debug.h"
#include "StringUtil.h"

std::string g_logFilePath(DEBUG_LOG_FILENAME);

void SetLogPath()
{
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	char timeStr[100];
	sprintf_s(timeStr, 100, "%02u-%02u-%02u",
		localTime.wHour,
		localTime.wMinute,
		localTime.wSecond
	);

	g_logFilePath += timeStr;
	g_logFilePath += ".txt";
}

// Debug Helpers
#define DEBUG_BUFFER_SIZE 8192

std::mutex __debug_mutex;

void DebugInit()
{
#ifdef DEBUG_LOGGING
	SetLogPath();

#if DEBUG_LOG_CREATE_NEW_ON_LAUNCH == 1
	// Delete logfile, ignoring failures
	remove(g_logFilePath.c_str());
#endif
#endif
}

std::string DebugWrite(const char* format, ...)
{
	std::lock_guard<std::mutex> lock(__debug_mutex);
	static char msgbuffer[DEBUG_BUFFER_SIZE];
	std::string returnLine{};

	va_list args;
	va_start(args, format);
	vsprintf_s(msgbuffer, DEBUG_BUFFER_SIZE, format, args);
	va_end(args);

	std::string buffer(DEBUG_LOG_ENTRY_PREFIX);
	buffer += msgbuffer;
	OutputDebugStringA(buffer.c_str());

	// Note the log time
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	char timeStr[100];
	sprintf_s(timeStr, 100, "%02u:%02u:%02u.%03u ",
		localTime.wHour,
		localTime.wMinute,
		localTime.wSecond,
		localTime.wMilliseconds); // Format: "hh:mm:ss.ms"

#ifdef DEBUG_LOGGING
	// Log the string to a file
	FILE* file = nullptr;

	errno_t err = fopen_s(
		&file,
		g_logFilePath.c_str(),
		"at+"
	);

	if (err != 0)
	{
		std::string err_msg("Unable to open log file: ");
		err_msg += std::to_string(err);
		err_msg += "\n";
		DEBUGLOG(err_msg.c_str());
	}
	else
	{
		std::string fileStr{};
		fileStr += std::to_string(GetCurrentThreadId());
		fileStr += " \t";
		fileStr += timeStr;
		fileStr += msgbuffer;

		fwrite(
			(void*)fileStr.c_str(),
			sizeof(char),
			fileStr.length(),
			file
		);

		fflush(file);
		fclose(file);
	}
#endif

	// Assemble return line
	returnLine += timeStr;
	returnLine += msgbuffer;

	return returnLine;
}
