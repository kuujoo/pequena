#include "pequena/log.h"
#include "pequena/time.h"
#include <chrono>
#include <iostream>
#include <ctime>

namespace
{
	std::vector<std::shared_ptr<peq::log::ILog>> allLoggers;

	const std::string dateAndTime()
	{
		char buffer[80] = { 0 };
		time_t t = peq::time::epochS();
		auto timeinfo = localtime(&t);
		strftime (buffer,80,"%c",timeinfo);
		return std::string(buffer);
	}
}


void peq::log::ConsoleLog::message(const std::string& str)
{
	std::cout << str << std::endl;
}

void peq::log::ConsoleLog::warning(const std::string& str) 
{
	std::cout << str << std::endl;
}

void peq::log::ConsoleLog::error(const std::string& str)
{
	std::cout << str << std::endl;
}

void peq::log::ConsoleLog::debug(const std::string& str)
{
	std::cout << str << std::endl;
}


std::vector<std::shared_ptr<peq::log::ILog>>& peq::log::loggers()
{
	return allLoggers;
}

void peq::log::message(const std::string& msg)
{
	auto useMsg = "[message][" + dateAndTime() + "] " + msg;
	auto &ls = loggers();
	for (auto l : ls)
	{
		l->message(useMsg);
	}
}

void peq::log::error(const std::string& msg)
{
	auto useMsg = "[error][" + dateAndTime() + "] " + msg;
	auto &ls = loggers();
	for (auto l : ls)
	{
		l->error(useMsg);
	}
}

void peq::log::warning(const std::string& msg)
{
	auto useMsg = "[warning][" + dateAndTime() + "] " + msg;
	auto &ls = loggers();
	for (auto l : ls)
	{
		l->warning(useMsg);
	}
}

void peq::log::debug(const std::string& msg)
{
#ifdef _DEBUG
	auto useMsg = "[debug][" + dateAndTime() + "] " + msg;
	auto &ls = peq::log::loggers();
	for (auto l : ls)
	{
		l->debug(useMsg);
	}
#endif

}