#include <pequena/network/network.h>
#include <pequena/network/http/http.h>
#include <iostream>

using namespace peq;
using namespace peq::network;
using namespace peq::http;

class ApiSession : public HttpSession
{
public:
	ApiSession() : HttpSession()
	{
		// setup routes
		_router.set(Method::GET, "/", [](const Request& req) -> Response
		{
			return Response::createText(Status::OK, "ROOT");
		})
		// setup get parameters, types do not matter currently. requests with undefined parameters are rejected (returns BadRequest response)
		.set(Method::GET, "/api/test?id=int&name=string", [](const Request& req) -> Response
		{
			return Response::createText(Status::OK, "GET TEST");
		})
		.set(Method::POST, "/api/test", [](const Request& req) -> Response
		{
			return Response::createText(Status::OK, "POST TEST");
		})
		.setDefault([](const Request& req) -> Response
		{
			return Response::createText(Status::BadRequest, "Bad");
		});

		setKeepaliveTimeout(5);
	}
	~ApiSession()
	{
	}
	void connected() override
	{
		std::cout << "http session connected" << std::endl;
	}
	void httpRequestAvailable(const Request& http) override
	{
		Response resp = _router.route(http);
		send(resp);
	}
	void disconnected() override
	{
		std::cout << "http session disconnected" << std::endl;
	}
private:
	Router _router;
};

int main() {
	// initialize network things
	awake();

	Server server;
	server
		.setPort(8181)
		.setSessionHandler<ApiSession>() // configure session
		.setThreads(4) // use 4 threads
		.start(); // blocks forever, call .stop() to stop server

	// destroy network things
	destroy();
	return 0;
}