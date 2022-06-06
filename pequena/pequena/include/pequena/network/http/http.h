#pragma once

#include "../network.h"
#include "lhttp/llhttp.h"
#include <unordered_set>
#include <vector>
#include <map>

namespace peq
{
	namespace http
	{
		enum class Method
		{
			DELETE = 0,
			GET = 1,
			HEAD = 2,
			POST = 3,
			PUT = 4,
			CONNECT = 5,
			OPTIONS = 6,
			TRACE = 7,
			COPY = 8,
			LOCK = 9,
			MKCOL = 10,
			MOVE = 11,
			PROPFIND = 12,
			PROPPATCH = 13,
			SEARCH = 14,
			UNLOCK = 15,
			BIND = 16,
			REBIND = 17,
			UNBIND = 18,
			ACL = 19,
			REPORT = 20,
			MKACTIVITY = 21,
			CHECKOUT = 22,
			MERGE = 23,
			MSEARCH = 24,
			NOTIFY = 25,
			SUBSCRIBE = 26,
			UNSUBSCRIBE = 27,
			PATCH = 28,
			PURGE = 29,
			MKCALENDAR = 30,
			LINK = 31,
			UNLINK = 32,
			SOURCE = 33,
			PRI = 34,
			DESCRIBE = 35,
			ANNOUNCE = 36,
			SETUP = 37,
			PLAY = 38,
			PAUSE = 39,
			TEARDOWN = 40,
			GET_PARAMETER = 41,
			SET_PARAMETER = 42,
			REDIRECT = 43,
			RECORD = 44,
			FLUSH = 45
		};

		enum class Status
		{
			Continue = 100,
			SwitchingProtocols = 101,
			Processing = 102,
			EarlyHints = 103,
			OK = 200,
			Created = 201,
			Accepted = 202,
			NonAuthoritativeInformation = 203,
			NoContent = 204,
			ResetContent = 205,
			PartialContent = 206,
			MultiStatus = 207,
			AlreadyReported = 208,
			IMUsed = 226,
			MultipleChoices = 300,
			MovedPermanently = 301,
			Found = 302,
			SeeOther = 303,
			NotModified = 304,
			UseProxy = 305,
			TemporaryRedirect = 307,
			PermanentRedirect = 308,
			BadRequest = 400,
			Unauthorized = 401,
			PaymentRequired = 402,
			Forbidden = 403,
			NotFound = 404,
			MethodNotAllowed = 405,
			NotAcceptable = 406,
			ProxyAuthenticationRequired = 407,
			RequestTimeout = 408,
			Conflict = 409,
			Gone = 410,
			LengthRequired = 411,
			PreconditionFailed = 412,
			ContentTooLarge = 413,
			PayloadTooLarge = 413,
			URITooLong = 414,
			UnsupportedMediaType = 415,
			RangeNotSatisfiable = 416,
			ExpectationFailed = 417,
			ImATeapot = 418,
			MisdirectedRequest = 421,
			UnprocessableContent = 422,
			UnprocessableEntity = 422,
			Locked = 423,
			FailedDependency = 424,
			TooEarly = 425,
			UpgradeRequired = 426,
			PreconditionRequired = 428,
			TooManyRequests = 429,
			RequestHeaderFieldsTooLarge = 431,
			UnavailableForLegalReasons = 451,
			InternalServerError = 500,
			NotImplemented = 501,
			BadGateway = 502,
			ServiceUnavailable = 503,
			GatewayTimeout = 504,
			HTTPVersionNotSupported = 505,
			VariantAlsoNegotiates = 506,
			InsufficientStorage = 507,
			LoopDetected = 508,
			NotExtended = 510,
			NetworkAuthenticationRequired = 511,
		};



		struct Header
		{
			Header() {}
			static Header createKeepAliveResponse(unsigned seconds, unsigned requests);
			static Header createContentTypeResponse(const std::string& type);

			Header(const std::string& name, const std::string& value) : name(name), value(value) {}
			Header(const std::string& name, int value) : name(name), value(std::to_string(value)) {}
			std::string name;
			std::string value;
		};

		struct Version {
			int major;
			int minor;
		};

		struct Cookie
		{
			enum class SameSite
			{
				Strict,
				Lax,
				None
			};
			static Cookie create(const std::string& cookie);
			Cookie() = default;
			Cookie(const std::string& name, const std::string& value);
			std::string name =  "";
			std::string value = "";
			uint64_t maxAge = 0;
			std::string path;
			bool httpOnly = false;
			bool secure = false;
			SameSite sameSite = SameSite::Lax;
		};

		struct Authorization
		{
			enum class Type
			{
				Basic,
				Bearer
			};
			Authorization(const std::string& authenticationHeaderValue);
			Authorization();
			bool empty() const;
			std::string user;
			std::string password;
			Type type;
		};

		struct Apikey
		{
			Apikey(const std::string& key) : key(key)
			{
			}
			Apikey()
			{
			}
			std::string key;
		};

		class Parameters
		{
		public:
			Parameters() {}
			Parameters(const std::string& url);
			std::string s(const std::string& key) const;
			int i(const std::string& key) const;
			int64_t i64(const std::string& key) const;
			bool b(const std::string& key) const;
			void set(const std::string& key, const std::string& value);
			std::vector<std::string> keys() const;
		private:
			const std::string& get(const std::string& key) const;
			std::map<std::string, std::string> _params;
		};

		struct Url
		{
			Url(){}
			Url(const std::string& fullUrl);
			std::string full;
			std::string path;
			Parameters params;
		};

		struct Request
		{
			Method method;
			Url url;
			Version version;
			std::vector<Header> headers;
			peq::network::Data body;
			const Authorization auth() const;
			const Apikey apiKey() const;
			const Cookie cookie(const std::string& cookie) const;
			peq::network::SocketInfo info;
			bool secure = false;
			bool wantsToKeepAlive() const;
			bool wantsToClose() const;
		};

		struct Response
		{
			static Response createEventStream(Status status);
			static Response createText(Status status, const std::string& content);
			static Response createJson(Status status, const std::string& content);
			static Response createFromFilename(Status status, const std::string& filename, peq::network::Data&& content);
			static Response create(Status status, const std::string& type, peq::network::Data&& content);
			static Response create(Status status, const std::string& type);
			Response() {}
			Response(Status status, peq::network::Data&& content);
			Response(Status status);
			void setCookie(const Cookie& cookie);
			Status status;
			http::Version version;
			std::vector<Header> headers;
			peq::network::Data body;
		};

		class Router
		{
		public:
			Router& setDefault(std::function<Response(const Request&)> func);
			Router& set(Method method, const std::string& route,
				std::function<Response(const Request&)> func, std::function<bool(const http::Request&)> authFunc = nullptr);
			Response route(const Request& request);
		private:
			struct Handle
			{
				std::string path;
				Method method;
				std::function<Response(const Request&)> func;
				std::function<bool(const http::Request&)> authFunc;
				std::vector<std::string> params;
			};
			std::vector<Handle> _handles;
			std::function<Response(const Request&)> _defaultFunc;
		};
	}
	namespace network
	{	
		struct ParseState
		{
			http::Header currentHeader;
			std::vector<http::Header> headers;
			http::Method method;
			http::Url url;
			http::Version version;
			const char* bodyStart = nullptr;
			unsigned bodyLength = 0;
			bool complete = false;
		};

		class HttpSession : public Session
		{
		public:
			static const int InfiniteKeepAlive = 0;
			HttpSession();
			virtual ~HttpSession();
			virtual void httpRequestAvailable(const http::Request& http) = 0;
		protected:
			void setKeepaliveTimeout(unsigned seconds);
			void setKeepaliveMaxRequests(unsigned maxRequests);
			int send(http::Response& response);
			int send(http::Response&& response);
			void update() override final;
			void resetIdle();
			int send(const char* data, unsigned dataLength) override;
		private:
			void dataAvailable() override;
			bool _keepAlive;
			unsigned _keepAliveTimeout;
			unsigned _timeout;
			unsigned _keepAlivemMaxRequests;
			unsigned _requests;
			uint64_t _idleStarted;

			http::Request _currentRequest;
			char _buffer[peq::network::receiveBufferSize];
			llhttp_t _parser;
			llhttp_settings_t _settings;
			ParseState _parseState;
			static int handleOnHeaderField(llhttp_t* h, const char* at, size_t length);
			static int handleOnHeaderValue(llhttp_t* h, const char* at, size_t length);
			static int handleOnMessageComplete(llhttp_t* h);
			static int handleOnHeadersComplete(llhttp_t* h);
			static int handleOnHeaderValueComplete(llhttp_t* h);
			static int handleOnHeaderFieldComplete(llhttp_t* h);
			static int handleOnBody(llhttp_t* h, const char* at, size_t length);
			static int handleOnUrl(llhttp_t* h, const char* at, size_t length);
		};
	}
}
