#pragma once

namespace peq
{
	namespace concurrency
	{
		class ITask
		{
		public:
			virtual void awake() = 0;
			virtual void execute() = 0;
			virtual void destroy() = 0;
		};
	}
}
