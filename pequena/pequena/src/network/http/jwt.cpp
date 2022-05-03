#include <pequena/network/http/jwt.h>
#include <pequena/crypto/crypto.h>
#include <pequena/stringutils.h>
#include <string>

using namespace peq;

namespace
{
	std::vector<uint8_t> signatureJWTHS256(const std::string& b64header, const std::string& b64payload, const std::string& secret)
	{
		std::stringstream s;
		s << b64header << "." << b64payload;
		auto data = s.str();
		jwt::Signature hmac(32, 0);
		if (peq::crypto::hmac::sha256(secret.c_str(), secret.size(), data.c_str(), data.size(), hmac.data(), 32) == 32)
		{
			return hmac;
		}
		return jwt::Signature();
	}
}

jwt::Token::Token()
{
	_header["alg"] = "HS256";
	_header["typ"] = "JWT";
}

std::optional<jwt::Token> jwt::Token::create(const std::string& tokenstr)
{
	jwt::Token token;
	std::string tmp;
	int i = 0;
	try
	{
		peq::string::foreachLine(tokenstr, '.', [&i, &token](const std::string& str)
		{
			// true == continue loop
			// false == break loop

			auto decoded = peq::string::from(peq::crypto::base64UrlDecode(str));
			if (decoded.empty())
			{
				return false;
			}

			if (i == 0) {
				token._header = nlohmann::json::parse(decoded);
			}
			else if (i == 1) {
				token._payload = nlohmann::json::parse(decoded);
			}
			else if (i == 2) {
				token._signature = Signature(decoded.size(), 0);
				memcpy(token._signature.data(), decoded.c_str(), 32);
			}
			else
			{
				return false;
			}
			i++;
			return true;
		});
	}
	catch (std::exception& e)
	{
		return std::optional<jwt::Token>();
	}

	if (i != 3)
	{
		return std::optional<jwt::Token>();
	}

	return token;
}

const std::string jwt::Token::b64Header() const
{
	auto str = _header.dump();
	auto b64 = peq::crypto::base64UrlEncode(str.data(), str.size());
	while (!b64.empty() && (b64.back() == '=' || b64.back() == '.'))
	{
		b64.pop_back();
	}
	return b64;
}

const std::string jwt::Token::jsonHeader() const
{
	auto str = _header.dump(4);
	return str;
}

const std::string jwt::Token::b64Payload() const
{
	auto str = _payload.dump();
	auto b64 = peq::crypto::base64UrlEncode(str.data(), str.size());
	while (!b64.empty() && (b64.back() == '=' || b64.back() == '.'))
	{
		b64.pop_back();
	}
	return b64;
}

const std::string jwt::Token::jsonPayload() const
{
	auto str = _payload.dump(4);
	return str;
}

const jwt::Signature& jwt::Token::signature() const
{
	return _signature;
}

std::string jwt::Token::alg() const
{
	std::string  str = _header["alg"];
	return str;
}

std::string jwt::Token::typ() const
{
	std::string str = _header["typ"];
	return str;
}
#include <iostream>
jwt::Token& jwt::Token::claim(const std::string& key, const std::string& value)
{
	_payload[key] = value;
	return *this;
}

jwt::Token& jwt::Token::claim(const std::string& key, int value)	
{
	_payload[key] = value;
	return *this;
}

jwt::Token& jwt::Token::claim(const std::string& key, int64_t value)
{
	_payload[key] = value;
	return *this;
}

jwt::Token& jwt::Token::claim(const std::string& key, uint64_t value)	
{
	_payload[key] = value;
	return *this;
}
#include <base64/base64.h>
std::string peq::jwt::sign(const jwt::Token& token, const std::string& secret)
{
	auto b64header = token.b64Header();
	auto b64payload = token.b64Payload();


	std::stringstream s;
	s << b64header << "." << b64payload;

	if (token.typ() == "JWT" && token.alg() == "HS256")
	{
		auto si = signatureJWTHS256(b64header, b64payload, secret);

		auto b64Signature = peq::crypto::base64UrlEncode((unsigned char*)si.data(), si.size());;

		while (!b64Signature.empty() && (b64Signature.back() == '=' || b64Signature.back() == '.'))
		{
			b64Signature.pop_back();
		}
		s << "." << b64Signature;
		return s.str();
	}
	return "";
}

bool peq::jwt::verify(const peq::jwt::Token& token, const std::string& secret)
{
	auto claimedSignature = token.signature();
	Signature trueSignature;
	auto b64header = token.b64Header();
	auto b64payload = token.b64Payload();

	if (token.typ() == "JWT" && token.alg() == "HS256")
	{
		trueSignature = signatureJWTHS256(b64header, b64payload, secret);
	}

	if (trueSignature.empty()) return false;
	if (claimedSignature.empty()) return false;
	if (claimedSignature.size() != trueSignature.size()) return false;

	return memcmp(claimedSignature.data(), trueSignature.data(), trueSignature.size()) == 0;
}