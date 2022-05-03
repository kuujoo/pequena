#pragma once

#include <vector>
#include <memory>
#include <string>

namespace peq
{
	namespace log
	{
		class ILog
		{
		public:
			virtual ~ILog() = default;
			virtual void message(const std::string& str) = 0;
			virtual void warning(const std::string& str) = 0;
			virtual void error(const std::string& str) = 0;
			virtual void debug(const std::string& str) = 0;
		};

		class ConsoleLog : public ILog
		{
			public:
			~ConsoleLog() = default;
			void message(const std::string& str) override;
			void warning(const std::string& str) override;
			void error(const std::string& str)  override;
			void debug(const std::string& str)  override;
		};

		std::vector<std::shared_ptr<ILog>>& loggers();
		template<typename T>
		void reg()
		{
			auto& logs = loggers();
			logs.push_back(std::shared_ptr<ILog>(new T()));
		}

		void message(const std::string&);
		void error(const std::string&);
		void warning(const std::string&);
		void debug(const std::string&);

	}
}