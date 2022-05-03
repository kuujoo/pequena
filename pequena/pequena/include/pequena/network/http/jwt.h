#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>
#include <optional>

namespace peq
{
	namespace jwt
	{
		namespace claims
		{
			// https://www.iana.org/assignments/jwt/jwt.xhtml#claims

			//Reserved claims
			constexpr const char* issuer = "iss";
			constexpr const char* subject = "sub";
			constexpr const char* audience = "aud";
			constexpr const char* expirationTime = "exp";
			constexpr const char* notBeforeTime = "nbf";
			constexpr const char* issuedAtTime = "iat";
			constexpr const char* jwtId = "jti";
		}

		using Signature = std::vector<uint8_t>;

		class Token
		{
		public:
			struct Claim
			{
				std::string key;
				std::string value;
			};

			static std::optional<jwt::Token> create(const std::string& token);
			Token();
			const std::string b64Header() const;
			const std::string jsonHeader() const;
			const std::string b64Payload() const;
			const std::string jsonPayload() const;
			const Signature& signature() const;
			//
			std::string alg() const;
			std::string typ() const;
			//
			Token& claim(const std::string& key, const std::string& value);
			Token& claim(const std::string& key, int value);
			Token& claim(const std::string& key, uint64_t value);
			Token& claim(const std::string& key, int64_t value);

			template<typename T>
			std::optional<T> get(const std::string& key) const
			{
				if (!_payload.contains(key)) return std::optional<T>();
				
				try
				{
					T v = _payload[key];
					return std::optional<T>(v);
				}
				catch (nlohmann::json::exception&)
				{
					return std::optional<T>();
				}
			}
		private:
			nlohmann::json _header;
			nlohmann::json _payload;
			Signature _signature;
		};

		std::string sign(const Token& token, const std::string& secret);
		bool verify(const Token& token, const std::string& secret);
	}
}