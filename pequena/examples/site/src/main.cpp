#include <pequena/network/network.h>
#include <pequena/network/http/http.h>
#include <pequena/network/http/files.h>
#include <iostream>

using namespace peq;
using namespace peq::network;
using namespace peq::http;

namespace
{
	// content is web-root-folder
	// pxl::http::Files maps web-requests to files
	peq::http::Files s_httpRoot("content");
}

class FileHttpSession : public HttpSession
{
public:
	FileHttpSession() : HttpSession() {
		setKeepaliveTimeout(5);
	}
	~FileHttpSession()
	{
	}
	void connected() override
	{
		std::cout << "http session connected" << std::endl;
	}
	void httpRequestAvailable(const Request& http) override
	{
		std::cout << "GET: " << http.url.path << std::endl;
		// get filepath based on url
		auto file = s_httpRoot.getPath(http.url.path);
		if (file.empty())
		{
			// No file, send NotFound
			send(Response(Status::NotFound));
		}
		else
		{
			// file exists, load & send
			auto content = s_httpRoot.get(file);
			send(Response::createFromFilename(Status::OK, file, std::move(content)));
		}
	}
	void disconnected() override
	{
		std::cout << "http session disconnected" << std::endl;
	}
};

int main() {
	// initialize network things
	peq::network::awake();

	peq::network::Server server;
	server
		.setPort(8181)
		.setSessionHandler<FileHttpSession>() // configure session
		.setThreads(4) // use 4 threads
		.start(); // blocks forever, call .stop() to stop server

	// destroy network things
	peq::network::destroy();
	return 0;
}