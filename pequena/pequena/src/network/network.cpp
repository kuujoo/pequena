#include "pequena/network/network.h"
#include "pequena/log.h"
#include <assert.h>
#include <deque>
#include <mutex>
#include <future>
#include <thread>
#include <iostream>
#include <sstream>

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

ServerSocketRef ServerSocket::create(int port)
{
	return createServerSocket(port);
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
	_selector = createSocketSelector();
}

void Server::ConnectionTask::awake()
{
	_selector->m_readyRead = std::bind(&ConnectionTask::readyRead, this, std::placeholders::_1);
}

void Server::ConnectionTask::abort()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_selector->m_readyRead = std::bind(&ConnectionTask::readyRead, this, std::placeholders::_1);
	_condition.notify_one();
}

void Server::ConnectionTask::execute()
{
	while (!_abort)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_condition.wait(lock, [this]() {
			return !_sockets.empty() || _abort || !_newSockets.empty();
		});

		for (auto it : _newSockets)
		{
			_sockets.push_back(it);
			_selector->add(it);
			auto handler = _handlers.find(it->id());
			if (handler != _handlers.end())
			{
				handler->second->connected();
			}
			
		}
		_newSockets.clear();

		_selector->wait(10);

		for (auto it : _sockets)
		{
			auto handler = _handlers.find(it->id());
			handler->second->update();

			if (it->isDisconnected())
			{
				
				auto handler = _handlers.find(it->id());
				if (handler != _handlers.end())
				{
					handler->second->disconnected();
				}

				_dcSockets.push_back(it);
			}
		}

		for (auto it : _dcSockets)
		{
			_selector->remove(it);
			_sockets.erase(std::remove(_sockets.begin(), _sockets.end(), it));
			_handlers.erase(it->id());
		}

		_dcSockets.clear();
	}
}

void Server::ConnectionTask::destroy()
{
	_selector->m_readyRead = nullptr;
}

void Server::ConnectionTask::add(ClientSocketRef socket,  SessionRef handler)
{
	std::unique_lock<std::mutex> lock(_mutex);
	_newSockets.push_back(socket);
	if (handler != nullptr)
	{
		_handlers[socket->id()] = handler;
	}
	_condition.notify_one();
}

void Server::ConnectionTask::readyRead(ClientSocketRef socket)
{
	auto handler = _handlers.find(socket->id());
	if (handler != _handlers.end())
	{
		handler->second->doHandle();
	}
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
	if (!_sertificates)
	{
		_sertificates = SertificateContainer::create();
	}
	_sertificates->add(crt, key);
	_tls = true;
	return *this;
}

Server& Server::setTLS(const std::string& pem)
{
	if (!_sertificates)
	{
		_sertificates = SertificateContainer::create();
	}
	_sertificates->addPem(pem);
	_tls = true;
	return *this;
}

Server::Server() : _threads(1), _port(80), _tls(false)
{
	_stop.store(false);
}

void Server::start()
{

	auto listenSocket = ServerSocket::create(_port);
	if (!listenSocket) {
		peq::log::error("Could not bind server to port: " + std::to_string(_port));
		return;
	}

	bool tls = _tls;

	_runner
		.setThreads(_threads)
		.start();

	int currentPool = 0;


	while (!_stop.load())
	{
		auto clientSocket = listenSocket->accept();
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
