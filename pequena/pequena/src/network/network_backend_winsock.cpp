 #include "pequena/network/network.h"
#include "pequena/log.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

using namespace peq;
using namespace peq::network;

namespace
{
	WSADATA wsaData;
	bool winsockInited = false;
	void initWinSock()
	{
		if (!winsockInited)
		{
			auto result = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (result != 0)
			{
				peq::log::error("WSAStartup failed with error: " + peq::string::from(result) );
				return;
			}
			winsockInited = true;
		}
	}
	void cleanupWinsock()
	{
		WSACleanup();
	}
}

class WINSOCKClientSocket : public ClientSocket
{
public:
	WINSOCKClientSocket(SOCKET socket, SocketMode mode) : _socket(socket), _mode(mode)
	{
		_id = static_cast<unsigned>(_socket);
	}
	~WINSOCKClientSocket()
	{
		disconnect();
	}

	void disconnect() override
	{
		if (_socket != INVALID_SOCKET)
		{
			shutdown(_socket, SD_BOTH);
			closesocket(_socket);
			_socket = INVALID_SOCKET;
		}
	}
	bool isDisconnected() const override
	{
		return _socket == INVALID_SOCKET;
	}
	unsigned id() const override
	{
		return _id;
	}
	int receive(char* data, unsigned dataLength) override
	{
		auto result = ::recv(_socket, data, dataLength, 0);
		if (result == SOCKET_ERROR)
		{
			auto error = WSAGetLastError();
			if (_mode == SocketMode::NonBlocking && error != WSAEWOULDBLOCK)
			{
				if (error == WSAECONNRESET || error == WSAECONNABORTED)
				{
					_socket = INVALID_SOCKET;
				}
			}	
		}
		if (result == 0)
		{
			disconnect();
		}
		return result;
	}
	int send(const char* data, unsigned dataLength) override
	{
		int sent = 0;
		int left = dataLength;
		do
		{
			auto result = ::send(_socket, (char*)data + dataLength - left, left, 0);
			if (result > 0) {
				left -= result;
				sent += result;
			}
			if (result == SOCKET_ERROR) {
				auto error = WSAGetLastError();
				if (_mode == SocketMode::NonBlocking && error != WSAEWOULDBLOCK)
				{
					if (error == WSAECONNRESET || error == WSAECONNABORTED)
					{
						_socket = INVALID_SOCKET;
					}
					return -1;
				}
			}
		} while (left > 0);

		return sent;
	}
	SocketInfo info() const
	{
		sockaddr_storage ci = { 0 };
		int len = sizeof(ci);
		int addrsize = sizeof(ci);
		getpeername(_socket, (sockaddr*)&ci, &len);
		SocketInfo i;
		if (ci.ss_family == AF_INET)
		{
			char address[INET_ADDRSTRLEN] ={0};
			in_addr ip4 = reinterpret_cast<const sockaddr_in*>(&ci)->sin_addr;
			unsigned short port = ntohs(((struct sockaddr_in*)&ci)->sin_port);
			inet_ntop(AF_INET, &ip4, address, sizeof(address));
			i.address = address;
		}
		else if (ci.ss_family == AF_INET6)
		{
			char address[INET6_ADDRSTRLEN] ={0};
			in6_addr addr = ((struct sockaddr_in6*)&ci)->sin6_addr;
			unsigned short port = ntohs(((struct sockaddr_in6*)&ci)->sin6_port);
			unsigned char* bytes = reinterpret_cast<unsigned char*>(&addr);
			if (IN6_IS_ADDR_V4MAPPED(&addr))
			{
				snprintf(address, sizeof(address), "%d.%d.%d.%d", bytes[12], bytes[13], bytes[14], bytes[15]);
			}
			else
			{
				inet_ntop(AF_INET6, &addr, address, sizeof(address));
			}
			i.address = address;
		}

		return i;
	}
private:
	unsigned _id = 0;
	SOCKET _socket = INVALID_SOCKET;
	SocketMode _mode;
};


class WINSOCKServerSocket : public ServerSocket
{
public:
	WINSOCKServerSocket(int port, SocketMode mode) : _id(0), _mode(mode)
	{
		struct addrinfo hints;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		{
			auto p = std::to_string(port);
			auto result = getaddrinfo(NULL, p.c_str(), &hints, &_info);
			if (result != 0) {
				peq::log::error("[WINSOCKSERVER] getaddrinfo failed");
				return;
			}
		}

		_socket = socket(_info->ai_family, _info->ai_socktype, _info->ai_protocol);
		_id = static_cast<unsigned>(_socket);

		if (_socket == INVALID_SOCKET)
		{
			peq::log::error("[WINSOCKSERVER] socket failed with error:" + peq::string::from(WSAGetLastError()));
			return;
		}
	
		{
			ULONG nonblocking = mode == SocketMode::NonBlocking ? 0 : 1;
			auto result = ioctlsocket(_socket, FIONBIO, &nonblocking);
			if (result == SOCKET_ERROR)
			{
				peq::log::error("[WINSOCKSERVER] block/nonblock set error:" + peq::string::from(WSAGetLastError()));
				closesocket(_socket);
				return;
			}
		}

		{
			auto v = 5000;	// in milliseconds
			auto result = setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&v, sizeof(v));
			if (result == SOCKET_ERROR)
			{
				peq::log::error("[WINSOCKSERVER] timeout set error:" + peq::string::from(WSAGetLastError()));
				closesocket(_socket);
				return;	// Temporary
			}
		}
		{
			auto result = bind(_socket, _info->ai_addr, (int)_info->ai_addrlen);
			if (result == SOCKET_ERROR) {
				peq::log::error("[WINSOCKSERVER] bind error:" + peq::string::from(WSAGetLastError()));
				closesocket(_socket);
				return;
			}
		}
		{
			auto result = listen(_socket, SOMAXCONN);
			if (_socket == SOCKET_ERROR) {
				peq::log::error("[WINSOCKSERVER] listen error:" + peq::string::from(WSAGetLastError()));
				closesocket(_socket);
				return;
			}
		}

		
		_ok = true;
	}
	ClientSocketRef accept()
	{
		auto clientSocket = ::accept(_socket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET && WSAGetLastError() == WSAEWOULDBLOCK)
		{
			return ClientSocketRef();
		}
		if (clientSocket == INVALID_SOCKET) {
			peq::log::error("[WINSOCKSERVER] accept failed with error:" + peq::string::from(WSAGetLastError()));
			return ClientSocketRef();
		}
		// Winsock does nof offer any way to query if socket is set to blocking or non-blocking
		return ClientSocketRef(new WINSOCKClientSocket(clientSocket, _mode));
	}

	~WINSOCKServerSocket()
	{
		freeaddrinfo(_info);
		closesocket(_socket);
	}
	bool ok() const {
		return _ok;
	}
	unsigned id() const override
	{
		return _id;
	}
private:
	SocketMode _mode;
	unsigned _id;
	SOCKET _socket = INVALID_SOCKET;
	struct addrinfo* _info = nullptr;
	bool _ok = false;
};


class WINSOCKESelector : public SocketSelector
{
public:
	WINSOCKESelector() : SocketSelector(), _cancelUdpSocket(SOCKET_ERROR)
	{
		_cancelUdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		// Create udp socket that sends data to itself
		// This way we can cancel select-call
		// Maybe we should use some other api ?
		// This works for now
		struct sockaddr_in server;
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		server.sin_port = htons(0);
		if( bind(_cancelUdpSocket ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
		{
			peq::log::error("[WINSOCKSELECTOR] could not bind selector loopback socket:" + peq::string::from(WSAGetLastError()));
			closesocket(_cancelUdpSocket);
			return;
		}

		struct sockaddr_in name;
		int nameLen = sizeof(name);
		getsockname(_cancelUdpSocket, (struct sockaddr *)&name, &nameLen);
		if (connect(_cancelUdpSocket, (struct sockaddr *)&name, nameLen) == SOCKET_ERROR)
		{
			peq::log::error("[WINSOCKSELECTOR] could not connect selector loopback socket:" + peq::string::from(WSAGetLastError()));
			closesocket(_cancelUdpSocket);
			return;
		}
	}
	void add(SocketRef socket) override
	{
		_sockets.push_back(socket);
	}

	std::vector<SocketRef> wait(unsigned timeoutms) override
	{
		std::vector<SocketRef> readyReadSockets;

		if (_sockets.empty())
		{
			return readyReadSockets;
		}

		TIMEVAL tv = { 0 };
		tv.tv_usec = static_cast<long>(timeoutms) * 1000;
		tv.tv_sec = 0;

		fd_set readFds;
		FD_ZERO(&readFds);
		std::map<SOCKET, SocketRef> socketMap;
		for (auto it : _sockets)
		{
			auto s = it.lock();
			FD_SET( (SOCKET)s->id(), &readFds);

			socketMap[(SOCKET)s->id()] = s;
		}

		if (_cancelUdpSocket != INVALID_SOCKET)
		{
			FD_SET(_cancelUdpSocket, &readFds);
		}

		auto result = select(readFds.fd_count, &readFds, nullptr, nullptr, &tv);
		if (result < 0) {
			peq::log::error("[WINSOCKSELECTOR] select failed with error:" + peq::string::from(WSAGetLastError()));
			return readyReadSockets;
		}

		if (result == 0)
		{
			return readyReadSockets;
		}

		for (int i = 0; i < readFds.fd_count; i++)
		{
			if ( FD_ISSET(readFds.fd_array[i], &readFds) ) 
			{	
				if (readFds.fd_array[i] == _cancelUdpSocket)
				{
					// drain socket
					char inBuf[100] = { 0 };
					recv(_cancelUdpSocket, inBuf, 100, 0);
				}
				else
				{
					auto s = socketMap.find(readFds.fd_array[i]);
					if (s == socketMap.end()) {
						continue;
					}
					readyReadSockets.push_back(s->second);
				}
			}
		}

		return readyReadSockets;
	}
	void remove(SocketRef socket) override
	{
		_sockets.erase(std::remove_if(_sockets.begin(), _sockets.end(), [socket](std::weak_ptr<Socket> s)->bool
		{
			return socket == s.lock();
		}));
	}

	void wakeUp() override
	{
		char* s = "wakeup";
		send(_cancelUdpSocket, s, 6,0);
	}
private:

	std::vector<std::weak_ptr<peq::network::Socket>> _sockets;
	SOCKET _cancelUdpSocket;
};

void peq::network::awake()
{
	initWinSock();
}

void peq::network::destroy()
{
	cleanupWinsock();
}

ServerSocketRef peq::network::createServerSocket(int port, SocketMode mode)
{
	auto sock = std::shared_ptr<WINSOCKServerSocket>( new WINSOCKServerSocket(port, mode) );
	if (!sock->ok())
	{
		return std::shared_ptr<ServerSocket>();
	}
	return sock;
}

SocketSelectorRef peq::network::createSocketSelector()
{
	return SocketSelectorRef(new WINSOCKESelector());
}


std::vector<Adapter> peq::network::adapters()
{
	
#define WINSOCK_MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define WINSOCK_FREE(x) HeapFree(GetProcessHeap(), 0, (x))

	PIP_ADAPTER_INFO pAdapterInfo;
	DWORD dwRetVal = 0;
	ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO *) WINSOCK_MALLOC(sizeof (IP_ADAPTER_INFO));
	if (pAdapterInfo == nullptr)
	{
		return std::vector<Adapter>();
	}

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		WINSOCK_FREE(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) WINSOCK_MALLOC(ulOutBufLen);
		if (pAdapterInfo == nullptr)
		{
			return std::vector<Adapter>();
		}
	}

	std::vector<Adapter> adapters;
	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		for (auto pAdapter = pAdapterInfo; pAdapter != nullptr; pAdapter = pAdapter->Next) {
			for (auto pAddr = &pAdapter->IpAddressList; pAddr != nullptr; pAddr = pAddr->Next) {
				Adapter adapter;
				adapter.address = pAddr->IpAddress.String;
				adapter.name = pAdapter->AdapterName;
				adapter.description = pAdapter->Description;
				adapter.mask = pAddr->IpMask.String;
				adapter.mac.length = std::min(size_t(pAdapter->AddressLength), sizeof(adapter.mac));
				memcpy(adapter.mac.bytes, pAdapter->Address, adapter.mac.length);
				adapters.push_back(adapter);
			}
		}
	}

	if (pAdapterInfo) {
		WINSOCK_FREE(pAdapterInfo);
	}

	return adapters;
}
