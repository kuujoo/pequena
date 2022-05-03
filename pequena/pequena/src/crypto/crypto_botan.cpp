#ifdef PEQ_BOTAN

#include <pequena/crypto/crypto.h>
#include <cstring>
#include <botan/mac.h>
#include <botan/hex.h>
#include <botan/base64.h>
#include <assert.h>

size_t peq::crypto::hmac::sha256(const void* key, const size_t keylen, const void* data,
	size_t datalen, void* out, const size_t outlen)
{
	std::unique_ptr<Botan::MessageAuthenticationCode> hmac = Botan::MessageAuthenticationCode::create("HMAC(SHA-256)");
	hmac->set_key((uint8_t*)key, keylen);
	hmac->update((uint8_t*)data, datalen);
	auto bytes = hmac->final();
	assert(bytes.size() == 32);
	assert(outlen == 32);
	memcpy(out, bytes.data(), std::min(outlen, bytes.size()));
	return bytes.size();
}


std::string peq::crypto::base64Encode(const void* data, size_t datalen)
{
	return Botan::base64_encode((uint8_t*)data, datalen);
}

std::vector<uint8_t> peq::crypto::base64Decode(const std::string& in)
{
	auto sv = Botan::base64_decode(in);
	std::vector<uint8_t> v(sv.size(), 0);
	memcpy(v.data(), sv.data(), sv.size());
	return v;
}

std::string peq::crypto::hex(const void* data, size_t datalen)
{
	return Botan::hex_encode((uint8_t*)data, datalen);
}

#endif
