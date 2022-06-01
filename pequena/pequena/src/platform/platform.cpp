#include "pequena/platform/platform.h"
#include <filesystem>

std::string peq::platform::path(peq::platform::Path p, const std::string& file)
{
	std::string pa = "";
	switch (p)
	{
	case::peq::platform::Path::Binary:
		pa = binaryPath();
		break;
	case::peq::platform::Path::SharedProgramData:
		pa = sharedProgramDataPath();
		break;
	}

	std::filesystem::path p0(pa);
	std::filesystem::path p1(file);
	return (p0 / p1).u8string();
}
