#ifndef PEQ_BOTAN

#include <pequena/crypto/crypto.h>
#include <hmac_sha256.h>
#include <sha256.h>
#include "base64/base64.h"
#include <cstring>

size_t peq::crypto::hmac::sha256(const void* key, const size_t keylen, const void* data,
	size_t datalen, void* out, const size_t outlen)
{
	 return hmac_sha256(key,  keylen, data, datalen, out,  outlen); 
}

std::string peq::crypto::hex(const void* data, size_t datalen)
{
	return "";
}

std::string peq::crypto::base64Encode(const void* data, size_t datalen)
{
	return base64_encode((const unsigned char*)data, size, false);
}

std::vector<uint8_t> peq::crypto::base64Decode(const std::string& in)
{
	auto d = base64_decode(in);
	std::vector<uint8_t> r(d.size(), 0);
	memcpy(r.data(), d.data(), d.size());
	return r;
}

#endif
