 #include "pequena/network/network.h"
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
				printf("WSAStartup failed with error: %d\n", result);
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
	WINSOCKClientSocket(SOCKET socket) : _socket(socket)
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
		if (result == 0 || result == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
		{
		}
		return result;
	}
	int send(const char* data, unsigned dataLength) override
	{
		auto result = ::send(_socket, (char*)data, dataLength, 0);
		if (result == SOCKET_ERROR)
		{
		}
		return result;
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
};


class WINSOCKServerSocket : public ServerSocket
{
public:
	WINSOCKServerSocket(int port)
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
				printf("getaddrinfo failed with error: %d\n", result);
				WSACleanup();
				return;
			}
		}

		_socket = socket(_info->ai_family, _info->ai_socktype, _info->ai_protocol);
		if (_socket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			return;
		}
		{
			auto v = 5000;	// in milliseconds
			auto result = setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&v, sizeof(v));
			if (result == SOCKET_ERROR)
			{
				printf("timeout error: %d\n", WSAGetLastError());
				return;	// Temporary
			}
		}
		{
			auto result = bind(_socket, _info->ai_addr, (int)_info->ai_addrlen);
			if (result == SOCKET_ERROR) {
				printf("bind failed with error: %d\n", WSAGetLastError());
				return;
			}
		}
		{
			auto result = listen(_socket, SOMAXCONN);
			if (_socket == SOCKET_ERROR) {
				printf("listen failed with error: %d\n", WSAGetLastError());
				closesocket(_socket);
				WSACleanup();
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
			printf("accept failed with error: %d\n", WSAGetLastError());
			WSACleanup();
			return ClientSocketRef();
		}

		return ClientSocketRef(new WINSOCKClientSocket(clientSocket));
	}

	~WINSOCKServerSocket()
	{
		freeaddrinfo(_info);
		closesocket(_socket);
	}
	bool ok() const {
		return _ok;
	}
private:
	SOCKET _socket = INVALID_SOCKET;
	struct addrinfo* _info = nullptr;
	bool _ok = false;
};


class WINSOCKESelector : public SocketSelector
{
public:
	WINSOCKESelector() : SocketSelector()
	{
		
	}
	void add(ClientSocketRef socket) override
	{
		_sockets.push_back(socket);
	}
	void wait(unsigned timeoutms) override
	{
		if (_sockets.empty())
		{
			return;
		}

		TIMEVAL tv = { 0 };
		tv.tv_usec = static_cast<long>(timeoutms) * 1000;
		tv.tv_sec = 0;

		fd_set readFds;
		FD_ZERO(&readFds);

		for (auto it : _sockets)
		{
			auto s = std::dynamic_pointer_cast<WINSOCKClientSocket>(it.lock());
			FD_SET( (SOCKET)s->id(), &readFds);
		}

		auto result = select(readFds.fd_count, &readFds, nullptr, nullptr, &tv);
		if (result < 0) {
			printf("select failed with error: %d\n", WSAGetLastError());			
		}

		if (result == 0)
		{
			return;
		}

		for (int i = 0; i < readFds.fd_count; i++)
		{
			if ( FD_ISSET(readFds.fd_array[i], &readFds) ) 
			{		
				if (m_readyRead)
				{
					auto socket = _sockets[i].lock();
					m_readyRead(socket);
				}
			}
		}
	}
	void remove(ClientSocketRef socket) override
	{
		_sockets.erase(std::remove_if(_sockets.begin(), _sockets.end(), [socket](std::weak_ptr<ClientSocket> s)->bool
		{
			return socket == s.lock();
		}));
	}
private:
	std::vector<std::weak_ptr<ClientSocket>> _sockets;
};

void peq::network::awake()
{
	initWinSock();
}

void peq::network::destroy()
{
	cleanupWinsock();
}

ServerSocketRef peq::network::createServerSocket(int port)
{
	auto sock = std::shared_ptr<WINSOCKServerSocket>( new WINSOCKServerSocket(port) );
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