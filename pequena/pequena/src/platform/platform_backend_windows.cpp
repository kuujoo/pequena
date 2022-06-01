#include "pequena/platform/platform.h"
#include "pequena/log.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <stdexcept>
#include <shlwapi.h>

#include "shlobj.h"
#include <assert.h>
#include <filesystem>

#pragma comment(lib,"shlwapi.lib")

using namespace peq;
using namespace peq::platform;

namespace
{
#ifdef UNICODE
	constexpr LPCWSTR keyPath = L"SOFTWARE\\Microsoft\\Cryptography";
	constexpr LPCWSTR machineGuid = L"MachineGuid";
	std::string utf8Encode(const std::wstring& utf16)
	{
		// Result of the conversion
		std::string utf8;

		// First, handle the special case of empty input string
		if (utf16.empty())
		{
			return utf8;
		}

		const int utf16Length = static_cast<int>(utf16.length());

		const int utf8Length = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16.data() ,utf16Length, nullptr, 0, nullptr, nullptr);
		if (utf8Length == 0)
		{
			return "";
		}

		// Make room in the destination string for the converted bits
		utf8.resize(utf8Length);

		// Do the actual conversion from UTF-16 to UTF-8
		int result = ::WideCharToMultiByte(CP_UTF8,	WC_ERR_INVALID_CHARS,utf16.data(), utf16Length, &utf8[0],utf8Length, nullptr, nullptr);
		if (result == 0)
		{
			return "";
		}

		return utf8;
	}
#else
	constexpr char* keyPath = "SOFTWARE\\Microsoft\\Cryptography";
	constexpr char* machineGuid = "MachineGuid";

	std::string utf8Encode(const std::string& str)
	{
		return str;
	}
#endif

	std::string machineIdStr;

	void readMachineId()
	{
		DWORD size = 0;
		if (RegGetValue(HKEY_LOCAL_MACHINE, keyPath, machineGuid, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS)
		{
			peq::log::warning("Error while reading machine id!");
			return;
		}

		if (size == 1)
		{
			peq::log::warning("Error while reading machine id!");
			return;
		}

		//bufferSize contains null terminator, so reserve size without it
#if UNICODE
		std::wstring str( (size/ sizeof(wchar_t)) - sizeof(wchar_t), (wchar_t)0);
#else
		std::string str((size / sizeof(char)) - sizeof(char), (char)0);
#endif

		if (RegGetValue(HKEY_LOCAL_MACHINE, keyPath, machineGuid, RRF_RT_REG_SZ, nullptr, str.data(), &size) != ERROR_SUCCESS)
		{
			peq::log::warning("Error while reading machine id!");
			return;
		}
		machineIdStr = utf8Encode(str);
	}
}

void peq::platform::awake()
{
	readMachineId();
}

const std::string peq::platform::machineId()
{
	if (machineIdStr.empty())
	{
		peq::log::warning("MachineId is empty! Remember to call platform::awake()");
		assert(0);
	}
	return machineIdStr;
}

std::string peq::platform::binaryPath()
{

	TCHAR str[MAX_PATH] ={0};
	GetModuleFileName(nullptr, str, MAX_PATH);
	auto utf8 = utf8Encode(str);
	auto slash = utf8.find_last_of("\\/");
	return utf8.substr(0, slash);
}

std::string peq::platform::sharedProgramDataPath()
{
	TCHAR szPath[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath);
	return utf8Encode(szPath);
}
