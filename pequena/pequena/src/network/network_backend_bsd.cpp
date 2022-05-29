#include "pequena/network/network.h"

using namespace peq;
using namespace peq::network;

void peq::network::awake()
{
	assert(0);
}

void peq::network::destroy()
{
	assert(0);
}

ServerSocketRef peq::network::createServerSocket(int port)
{
	assert(0);
	return nullptr;
}

SocketSelectorRef peq::network::createSocketSelector()
{
	assert(0);
	return nullptr;
}


std::vector<Adapter> peq::network::adapters()
{
	assert(0);
	std::vector<Adapter> adapters;
	return adapters;
}
