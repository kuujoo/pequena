#include "pequena/platform/platform.h"
#include "pequena/log.h"

using namespace peq;
using namespace peq::platform;


void peq::platform::awake()
{
}

const std::string peq::platform::machineId()
{
	assert(0);
	return "RETURN_VALID_LINUX_MACHINE_ID";
}

std::string peq::platform::sharedProgramDataPath()
{
	assert(0);
	return "";
}

std::string peq::platform::binaryPath()
{
	assert(0);
	return "";
}
