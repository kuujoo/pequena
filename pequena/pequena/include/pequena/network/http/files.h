#pragma once

#include "http.h"
#include <string>
#include <map>
#include <filesystem>

namespace peq
{
	namespace http
	{
		class Files
		{
		public:
			Files(const std::string& fileRoot);
			~Files();
			std::string getPath(const std::string& url) const;
			Data get(const std::string& path) const;
		private:
			std::string _root;
			std::map<std::string, std::filesystem::path> _paths;
		};
	}
}