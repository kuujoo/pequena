#ifndef PEQ_BOTAN

#include "pequena/network/network.h"

using namespace peq::network;

SessionFilterRef peq::network::createFilterTLS(SessionFilter::Mode mode, SertificateContainerRef sertificates)
{
	return nullptr;
}

SertificateContainerRef peq::network::createSertificateContainer()
{
	return nullptr;
}

#endif
