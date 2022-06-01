#pragma once

#include <string>

namespace peq
{
	namespace platform
	{
		enum class Path
		{
			Binary,
			SharedProgramData
		};

		void awake();
		const std::string machineId();

		// get absolute path to file from relative path
		std::string path(Path p, const std::string& file);

		// absolute path to binary
		std::string binaryPath();

		// absolute path to shared program data ( C:\ProgramData in Windows )
		std::string sharedProgramDataPath();
	}
}
