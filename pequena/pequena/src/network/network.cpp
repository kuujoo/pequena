#include "pequena/network/network.h"
#include "pequena/log.h"
#include <assert.h>
#include <deque>
#include <mutex>
#include <future>
#include <thread>
#include <iostream>
#include <sstream>
#include <filesystem>

using namespace peq;
using namespace peq::network;

std::optional<Mac> Mac::create(const std::string& str)
{
	Mac mac;
	if (str.size() == 17)
	{
		if (sscanf(str.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X", &mac.bytes[5], &mac.bytes[4], &mac.bytes[3], &mac.bytes[2], &mac.bytes[1], &mac.bytes[0]) == 6)
		{
			mac.length = 6;
			return mac;
		}
	}
	else if (str.size() == 24)
	{
		if (sscanf(str.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", &mac.bytes[7], &mac.bytes[6], &mac.bytes[5], &mac.bytes[4], &mac.bytes[3], &mac.bytes[2], &mac.bytes[1], &mac.bytes[0]) == 8)
		{
			mac.length = 8;
			return mac;
		}
	}
	return std::optional<Mac>();
}

SertificateContainerRef SertificateContainer::create()
{
	return createSertificateContainer();
}

SessionFilterRef SessionFilter::createTLS(SessionFilter::Mode mode, SertificateContainerRef sertificates)
{
	return createFilterTLS(mode, sertificates);
}

ServerSocketRef ServerSocket::create(int port, SocketMode mode)
{
	return createServerSocket(port, mode);
}

SocketSelectorRef SocketSelector::create() {
	return createSocketSelector();
}

int Session::receive(char* data, unsigned dataLength)
{
	if (_filter)
	{
		return _filter->receive(data, dataLength);
	}
	else
	{
		return socketReceive(data, dataLength);
	}
}

int Session::send(const char* data, unsigned dataLength)
{
	std::lock_guard<std::mutex> lock(m_sendMutex);
	if (_filter)
	{
		return _filter->send(data, dataLength);
	}
	else
	{
		return socketSend(data, dataLength);
	}
}

void Session::doHandle()
{
	if (_filter)
	{
		dataAvailable();
		while (_filter->hasData())
		{
			dataAvailable();
		}
	}
	else
	{
		return dataAvailable();
	}
}

int Session::socketReceive(char* data, unsigned dataLength)
{
	return _socket->receive(data, dataLength);
}


int Session::socketSend(const char* data, unsigned dataLength)
{
	return _socket->send(data, dataLength);
}

void Session::disconnect()
{
	_socket->disconnect();
}

void Session::bindFilter(SessionFilterRef filter)
{
	_filter = filter;
	filter->sendFunc = std::bind(&Session::socketSend, this, std::placeholders::_1, std::placeholders::_2);
	filter->recvFunc = std::bind(&Session::socketReceive, this, std::placeholders::_1, std::placeholders::_2);
}

Server::ConnectionTask::ConnectionTask() : _abort(false)
{
	
}

void Server::ConnectionTask::awake()
{
	_selector = createSocketSelector();
}

void Server::ConnectionTask::abort()
{
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_abort = true;
	}
	_selector->wakeUp();
	_condition.notify_one();
}

void Server::ConnectionTask::execute()
{
	std::vector<ClientSocketRef> sockets;
	std::map<unsigned, SessionRef> handlers;
	std::vector<ClientSocketRef> dcSockets;

	while (!_abort)
	{
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_condition.wait(lock, [this, &sockets]() {
				return !sockets.empty() || _abort || !_newSockets.empty();
			});

			for (auto it : _newSockets) {
				sockets.push_back(it.socket);
				_selector->add(it.socket);
				handlers[it.socket->id()] = it.handler;
				it.handler->connected();

			}
			_newSockets.clear();
		}

		auto readSockets = _selector->wait(1000);

		for (auto it : readSockets) {
			auto handler = handlers.find(it->id());
			if (handler != handlers.end())
			{
				handler->second->doHandle();
			}
		}

		for (auto it : sockets)
		{
			auto handler = handlers.find(it->id());
			handler->second->update();

			if (it->isDisconnected())
			{
				
				auto handler = handlers.find(it->id());
				if (handler != handlers.end())
				{
					handler->second->disconnected();
				}

				dcSockets.push_back(it);
			}
		}

		for (auto it : dcSockets)
		{
			_selector->remove(it);
			sockets.erase(std::remove(sockets.begin(), sockets.end(), it));
			handlers.erase(it->id());
		}

		dcSockets.clear();
	}
}

void Server::ConnectionTask::destroy()
{
}

void Server::ConnectionTask::add(ClientSocketRef socket,  SessionRef handler)
{
	{
		std::lock_guard<std::mutex> lock(_mutex);
		NewSocket ns;
		ns.handler = handler;
		ns.socket = socket;
		_newSockets.push_back(ns);
	}
	_selector->wakeUp();
	_condition.notify_one();
}

Server& Server::setThreads(unsigned threads)
{
	_threads = threads;
	return *this;
}

Server& Server::setPort(unsigned port)
{
	_port = port;
	return *this;
}

Server& Server::setTLS(const std::string& crt, const std::string &key)
{
	if (!std::filesystem::exists(crt) || !std::filesystem::exists(key))
	{
		peq::log::error("TLS private key does not exists. TLS is not used!");
		return *this;
	}

	if (!_sertificates)
	{
		_sertificates = SertificateContainer::create();
	}

	if (!_sertificates->add(crt, key))
	{
		_sertificates = nullptr;
		peq::log::error("TLS sertificate error. TLS is not used!");
		return *this;
	}

	_tls = true;
	return *this;
}

Server& Server::setTLS(const std::string& pem)
{
	if (!std::filesystem::exists(pem))
	{
		peq::log::error("TLS private key does not exists. TLS is not used!");
		return *this;
	}

	if (!_sertificates)
	{
		_sertificates = SertificateContainer::create();
	}

	if (!_sertificates->addPem(pem))
	{
		_sertificates = nullptr;
		peq::log::error("TLS sertificate error. TLS is not used!");
		return *this;
	}

	_tls = true;
	return *this;
}

Server::Server() : _threads(1), _port(80), _tls(false)
{
	_stop.store(false);
}

void Server::start()
{

	auto listenSocket = ServerSocket::create(_port, SocketMode::NonBlocking);
	if (!listenSocket) {
		peq::log::error("Could not bind server to port: " + std::to_string(_port));
		return;
	}

	bool tls = _tls;

	_runner
		.setThreads(_threads)
		.start();

	int currentPool = 0;
	peq::network::SocketSelectorRef selector = peq::network::createSocketSelector();
	selector->add(listenSocket);
	while (!_stop.load())
	{
		auto results = selector->wait(100);
		if (results.empty()) {
			continue;
		}
		auto clientSocket = std::dynamic_pointer_cast<peq::network::ServerSocket>(results.front())->accept();
		if (clientSocket)
		{
			peq::log::debug("Accepted connection");
			auto session = _sessionProvider->get();
			if (tls)
			{
				if (auto filter = SessionFilter::createTLS(SessionFilter::Mode::Server, _sertificates)) {
					session->bindFilter(filter);
				}
			}
			session->_socket = clientSocket;
			currentPool = (currentPool + 1) % _threads;
			auto task = _runner.get(currentPool);
			task->add(clientSocket, session);
		}
	}

	for (unsigned i = 0; i < _threads; i++)
	{
		_runner.get(i)->abort();
	}
	_runner.wait();
}

void Server::stop()
{
	_stop.store(true);
}
