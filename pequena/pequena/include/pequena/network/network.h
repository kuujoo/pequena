#pragma once
#include "pequena/concurrency/task.h"
#include "pequena/concurrency/runner.h"
#include "pequena/stringutils.h"
#include <vector>
#include <memory>
#include <string>
#include <deque>
#include <mutex>
#include <future>
#include <map>
#include <optional>

namespace peq {

	namespace network {

		constexpr unsigned receiveBufferSize = 2048;
		class Socket;
		class ServerSocket;
		class ClientSocket;
		class SessionFilter;
		class SertificateContainer;
		using SocketRef = std::shared_ptr<Socket>;
		using ServerSocketRef = std::shared_ptr<ServerSocket>;
		using ClientSocketRef = std::shared_ptr<ClientSocket>;
		using SessionFilterRef = std::shared_ptr<SessionFilter>;
		using SertificateContainerRef = std::shared_ptr<SertificateContainer>;
		
		
		struct Mac 
		{
			static std::optional<Mac> create(const std::string& str);
			uint8_t bytes[8] = {0};
			size_t length = 0;
		};

		struct Adapter
		{
			std::string address;
			std::string name;
			std::string description;
			std::string mask;
			Mac mac;
		};

		std::vector<Adapter> adapters();

		class SertificateContainer
		{
		public:
			static SertificateContainerRef create();
			virtual void addPem(const std::string& sertificate) = 0;
			virtual void add(const std::string& crt, const std::string &key) = 0;
		protected:
			SertificateContainer() {}
		};

		class SessionFilter
		{
		public:
			enum class Mode
			{
				Server,
				Client
			};
			static SessionFilterRef createTLS(Mode mode, SertificateContainerRef sertificates);
			virtual ~SessionFilter() = default;
			virtual unsigned send(const char* data, unsigned bytes) = 0;
			virtual unsigned receive(char* buffer, unsigned bytes) = 0;
			virtual bool hasData() const = 0;
			std::function<int(const char* buffer, unsigned bytes)> sendFunc;
			std::function<int(char* buffer, size_t bytes)> recvFunc;
		};

		class Socket {
		public:
			virtual unsigned id() const = 0;
		};

		

		class ServerSocket : public Socket
		{
		public:
			static ServerSocketRef create(int port);
			virtual ~ServerSocket() = default;
			ServerSocket(const ServerSocket&) = delete;
			ServerSocket& operator=(const ServerSocket&) = delete;
			ServerSocket& operator=(ServerSocket&&) = delete;
			virtual ClientSocketRef accept() = 0;
		protected:
			ServerSocket() = default;
		};

		struct SocketInfo
		{
			std::string address;
			unsigned port;
			Mac mac;
		};

		class ClientSocket : public Socket
		{
		public:
			ClientSocketRef create();
			virtual ~ClientSocket() = default;
			ClientSocket(const ClientSocket&) = delete;
			ClientSocket& operator=(const ClientSocket&) = delete;
			ClientSocket& operator=(ClientSocket&&) = delete;
			virtual int receive(char* data, unsigned dataLength) = 0;
			virtual int send(const char* data, unsigned dataLength) = 0;
			virtual void disconnect() = 0;
			virtual bool isDisconnected() const = 0;
			virtual SocketInfo info() const = 0;
		protected:
			ClientSocket() = default;
		};

		class SocketSelector;
		using SocketSelectorRef = std::shared_ptr<SocketSelector>;

		class SocketSelector
		{
		public:
			SocketSelectorRef create();
			virtual ~SocketSelector() {}
			virtual void add(SocketRef socket) = 0;
			virtual void remove(SocketRef socket) = 0;
			virtual std::vector<SocketRef> wait(unsigned timeoutms) = 0;
		};
		
		class Server;
		class ConnectionTask;

		using Data = std::vector<char>;

		class Session
		{
		public:
			virtual void connected() = 0;
			virtual void dataAvailable() = 0;
			virtual void disconnected() = 0;
		protected:
			int receive(char* data, unsigned dataLength);
			virtual int send(const char* data, unsigned dataLength);
			void disconnect();
			virtual void update() {};
			SocketInfo info() const
			{
				return _socket->info();
			}
			bool secure() const
			{
				return _filter != nullptr;
			}
		private:
			void doHandle();
			int socketReceive(char* data, unsigned dataLength);
			int socketSend(const char* data, unsigned dataLength);
			void bindFilter(SessionFilterRef filter);
			friend class Server;
			friend class ConnectionTask;
			ClientSocketRef _socket;
			SessionFilterRef _filter;
			std::mutex m_sendMutex;
		};

		using SessionRef = std::shared_ptr<Session>;
		
		class Server
		{
		private:
			class IProvideSessions
			{
			public:
				virtual ~IProvideSessions() {}
				virtual SessionRef get() = 0;
			};
			template<typename T>
			class SessionProvider : public IProvideSessions
			{
			public:
				SessionRef get() override
				{
					return SessionRef(new T());
				}
			};
			class ConnectionTask : public peq::concurrency::ITask
			{
			public:
				ConnectionTask();
				void awake() override;
				void execute() override;
				void destroy() override;
				void add(ClientSocketRef socket, SessionRef handler);
				void abort();
			private:
				std::condition_variable _condition;
				std::mutex _mutex;
				struct NewSocket
				{
					ClientSocketRef socket;
					SessionRef handler;
				};
				std::vector<NewSocket> _newSockets;
				bool _abort;
			};
		public:
			Server();
			void start();
			void stop();

			template<typename T = Session>
			Server& setSessionHandler()
			{
				_sessionProvider = std::unique_ptr<IProvideSessions>(new SessionProvider<T>());
				return *this;
			}
			Server& setThreads(unsigned threads);
			Server& setPort(unsigned port);
			Server& setTLS(const std::string& crt, const std::string& key);
			Server& setTLS(const std::string& pem);
		private:
			std::unique_ptr<IProvideSessions> _sessionProvider;
			peq::concurrency::Runner<ConnectionTask> _runner;
			SertificateContainerRef _sertificates;
			unsigned _threads;
			std::atomic<bool> _stop;
			unsigned _port;
			bool _tls;
		};

		void awake();
		void destroy();

		ServerSocketRef createServerSocket(int port);
		SocketSelectorRef createSocketSelector();
		//
		SessionFilterRef createFilterTLS(SessionFilter::Mode mode, SertificateContainerRef sertificates);
		SertificateContainerRef createSertificateContainer();
	}
}

namespace peq
{
	namespace string
	{
		template<>
		inline std::string from(const peq::network::Mac& val)
		{
			char str[24] = { 0 };
			if (val.length == 6)
			{
				sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
					(const char*)val.bytes[0], (const char*)val.bytes[1], (const char*)val.bytes[2], (const char*)val.bytes[3],
					(const char*)val.bytes[4], (const char*)val.bytes[5]);
			}
			else if (val.length == 8)
			{
				sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
					(const char*)val.bytes[0], (const char*)val.bytes[1], (const char*)val.bytes[2], (const char*)val.bytes[3],
					(const char*)val.bytes[4], (const char*)val.bytes[5], (const char*)val.bytes[6], (const char*)val.bytes[7]);
			}
			return std::string(str);
		}
	}
}
