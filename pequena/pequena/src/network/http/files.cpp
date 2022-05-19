#include <pequena/network/http/files.h>
#include <pequena/log.h>
#include <filesystem>
#include <fstream>

using namespace peq::http;

namespace
{
	std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
		return str;
	}

	std::string fsToUrl(const std::string& path, const std::string& root)
	{
#ifdef WIN32
		return replaceAll(path, "\\", "/").substr(root.size());
#else
		return path.substr(root.size());
#endif
	}
}

Files::Files(const std::string& fileRoot) : _root(fileRoot)
{
	try
	{
		for (const auto& e : std::filesystem::recursive_directory_iterator(_root.data()))
		{
			auto p = e.path();
			if (std::filesystem::is_directory(p)) continue;

			auto str = p.u8string();
			_paths[fsToUrl(str, _root)] = str;;
		}
	}
	catch (const std::exception& e)
	{
	}
}

Files::~Files()
{

}

std::string Files::getPath(const std::string& url) const
{
	if (url.empty() || url == "/")
	{
		auto filePath = _paths.find("/index.html");
		if (filePath == _paths.end()) return "";
		return filePath->second.u8string();
	}
	else
	{
		auto filePath = _paths.find(url);
		if (filePath == _paths.end()) return "";
		return filePath->second.u8string();
	}
}

peq::network::Data Files::get(const std::string& path) const
{
	std::ifstream fs(path, std::ios::in | std::ios::binary);
	if (!fs.is_open()) return peq::network::Data();

	fs.seekg(0, std::ios::end);
	auto l = fs.tellg();
	fs.seekg(0, std::ios::beg);
	peq::network::Data data;
	data.resize(l);
	fs.read(data.data(), l);
	return data;
}
