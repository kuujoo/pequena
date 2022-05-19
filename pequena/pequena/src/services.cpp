#include "pequena/services.h"

using namespace peq;
using namespace peq::services;

namespace 
{
	std::vector<std::shared_ptr<IService>> serviceContainer;
}



//
namespace peq
{
	namespace services
	{
		std::vector<std::shared_ptr<IService>>& container()
		{
			return serviceContainer;
		}
	}
}
