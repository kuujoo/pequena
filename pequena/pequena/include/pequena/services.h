#pragma once

#include <memory>
#include <vector>

namespace peq
{
	class IService
	{
	public:
		virtual ~IService() = default;
	};

	namespace services
	{
		template<typename T>
		std::shared_ptr<T> get()
		{
			extern std::vector<std::shared_ptr<IService>>& container();//cpp
			auto& serviceContainer = container();
			for (auto it : serviceContainer)
			{
				if (auto s = std::dynamic_pointer_cast<T>(it))
				{
					return s;
				}
			}
			return nullptr;
		}

		template<typename T> 
		void reg()
		{
			extern std::vector<std::shared_ptr<IService>>& container();//cpp
			auto& serviceContainer = container();
			serviceContainer.push_back(std::shared_ptr<IService>(new T()));
		}
	}
}
