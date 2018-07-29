#pragma once

#include <windows.h>
#include <fstream>
#include <experimental/filesystem>


static std::string narrow(std::wstring const& str)
{
	const auto char_len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.length(), nullptr, 0, nullptr, nullptr);

	if (char_len == 0)
		return "";

	std::string result;
	result.resize(char_len);
	WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.size(), const_cast<char*>(result.c_str()), char_len, nullptr,
	                    nullptr);
	return result;
}

static std::wstring widen(std::string const& str)
{
	const auto char_len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), nullptr, 0);

	if (char_len == 0)
		return L"";

	std::wstring result;
	result.resize(char_len);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), const_cast<wchar_t*>(result.c_str()), char_len);
	return result;
}

static std::ofstream *LogStream = nullptr;

#ifdef _VERBOSE


#define LOG(msg) *LogStream << msg << std::endl
#define LOG_ARCX(data, msg) //*(data->logger) << msg << std::endl;
#define N(str) narrow(str)
#define INIT_LOG(path) init_log(path)

inline void init_log(std::experimental::filesystem::path const& root)
{
	LogStream = new std::ofstream();
	const auto log_file = root / L"arcx.log";
	LogStream->open(log_file, std::ios_base::out | std::ios_base::binary);
}

#else

#define LOG(msg)
#define LOG_ARCX(data, msg)
#define N(str) ""

#define INIT_LOG(path)

#endif
