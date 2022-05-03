#include <pequena/crypto/crypto.h>
#include <base64/base64.h>

#include <assert.h>

using namespace peq;

peq::Uuid peq::crypto::uuid()
{
	const auto id = uuids::uuid_system_generator{}();
	assert(!id.is_nil());
	assert(id.version() == uuids::uuid_version::random_number_based);
	assert(id.variant() == uuids::uuid_variant::rfc);
	return id;
}

std::string peq::crypto::base64UrlEncode(const void* data, size_t datalen)
{
	return base64_encode((const unsigned char*)data, datalen, true);
}

std::vector<uint8_t> peq::crypto::base64UrlDecode(const std::string& in)
{
	auto d = base64_decode(in);
	std::vector<uint8_t> r(d.size(), 0);
	memcpy(r.data(), d.data(), d.size());
	return r;
}
