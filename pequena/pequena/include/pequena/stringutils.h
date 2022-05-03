#pragma once

#include <string>
#include <sstream>
#include <istream>
#include <locale>
#include <functional>
#include <algorithm>
#include <cctype>

namespace peq
{
	namespace string
	{
		const std::locale locale = std::locale::classic();

		inline void foreachLine(const std::string& str, char sep, std::function<bool(const std::string& str)> func)
		{
			std::string tmp;
			std::istringstream st(str);
			int i = 0;
			while (std::getline(st, tmp, sep))
			{
				if (!func(tmp))
				{
					break;
				}
			}
		}

		template<typename T>
		T to(const std::string& t)
		{
			std::istringstream ss(t);
			ss.imbue(locale);
			T val;
			ss >> val;
			return val;
		}

		template<typename T>
		std::string from(const T& val)
		{
			return std::to_string(val);
		}

		template <>
		inline int64_t to(const std::string& val)
		{
			return std::stoll(val);
		}

		template <>
		inline uint32_t to(const std::string& val)
		{
			return std::stoull(val);
		}

		inline std::string trimLeft(std::string s)
		{
			auto it = std::find_if(s.begin(), s.end(),
				[](char c)
				{
					return !std::isspace(c, locale);
				});
			s.erase(s.begin(), it);
			return s;
		}

		inline std::string trimRight(std::string s)
		{
			auto it = std::find_if(s.rbegin(), s.rend(),
				[](char c)
				{
					return !std::isspace(c, locale);
				});
			s.erase(it.base(), s.end());
			return s;
		}

		inline std::string trim(std::string s) {
			return trimLeft(trimRight(s));
		}

		inline bool isNumber(const std::string& str)
		{
			for (char const& c : str)
			{
				if (std::isdigit(c) == 0)
				{
					return false;
				}
			}
			return true;
		}
	}
}