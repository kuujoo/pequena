#pragma once

#define UUID_SYSTEM_GENERATOR
#include <uuid.h>
#include <pequena/stringutils.h>
#include <string>
#include <vector>


namespace peq
{
	using Uuid = uuids::uuid;

	namespace crypto
	{
		namespace hmac
		{
			size_t sha256(const void* key, const size_t keylen, const void* data, size_t datalen,
			void* out, const size_t outlen);

		}
		std::string base64Encode(const void* data, size_t datalen);
		std::string base64UrlEncode(const void* data, size_t datalen);
		std::vector<uint8_t> base64UrlDecode(const std::string& in);
		std::vector<uint8_t> base64Decode(const std::string& in);
		std::string hex(const void* data, size_t datalen);
		Uuid uuid();
	}

}

namespace peq
{
	namespace string
	{
		template<>
		inline std::string from(const Uuid& val)
		{
			return uuids::to_string(val);
		}

		template<>
		inline std::string from(const std::vector<uint8_t>& val)
		{
			std::string str(val.size(), '\0');
			memcpy(str.data(), val.data(), val.size());
			return str;
		}
	}
}