#ifdef PEQ_BOTAN

#include <pequena/network/network.h>
#include <botan/tls_server.h>
#include <botan/tls_client.h>
#include <botan/build.h>
#include <botan/parsing.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/data_src.h>
#include <botan/hex.h>
#include <botan/rsa.h>

#if defined(BOTAN_HAS_SYSTEM_RNG)
#include <botan/system_rng.h>
#endif

#include <botan/pkcs8.h>
#include <botan/credentials_manager.h>
#include <botan/x509self.h>

#include <iostream>
#include <fstream>

using namespace peq;
using namespace peq::network;

namespace
{
	class ServerCredentials : public Botan::Credentials_Manager
	{
	public:
		ServerCredentials()
		{
		}
		void add(const std::string& server_crt, const std::string& server_key)
		{
			Botan::System_RNG rng;// Ignored
			Certificate_Info cert;
			cert.key.reset(Botan::PKCS8::load_key(server_key, rng));
			Botan::DataSource_Stream in(server_crt);
			while (!in.end_of_data())
			{
				try
				{
					cert.certs.push_back(Botan::X509_Certificate(in));
				}
				catch (std::exception&)
				{

				}
			}
			_creds.push_back(cert);
		}

		std::string getFingerprint() {
			if (!_creds.empty() && !_creds[0].certs.empty())
				return _creds[0].certs[0].fingerprint();
			else
				return "";
		}

		std::vector<Botan::Certificate_Store*> trusted_certificate_authorities(const std::string& type, const std::string& /*hostname*/) override
		{
			// if client authentication is required, this function
			// shall return a list of certificates of CAs we trust
			// for tls client certificates, otherwise return an empty list
			return std::vector<Botan::Certificate_Store*>();
		}

		std::vector<Botan::X509_Certificate> cert_chain(const std::vector<std::string>& algos, const std::string& type, const std::string& hostname) override
		{
			// return the certificate chain being sent to the tls client
			// e.g., the certificate file "botan.randombit.net.crt"
			for (auto&& i : _creds)
			{
				if (std::find(algos.begin(), algos.end(), i.key->algo_name()) == algos.end())
					continue;

				if (hostname != "" && !i.certs[0].matches_dns_name(hostname))
					continue;

				return i.certs;
			}

			return std::vector<Botan::X509_Certificate>();
		}

		Botan::Private_Key* private_key_for(const Botan::X509_Certificate& cert, const std::string& type, const std::string& context) override
		{
			// return the private key associated with the leaf certificate,
			// in this case the one associated with "botan.randombit.net.crt"

			for (auto&& i : _creds)
			{
				if (cert == i.certs[0])
				{
					return i.key.get();
				}
			}

			return nullptr;
		}

	private:
		struct Certificate_Info
		{
			std::vector<Botan::X509_Certificate> certs;
			std::shared_ptr<Botan::Private_Key> key;
		};

		std::vector<Certificate_Info> _creds;
		std::vector<std::shared_ptr<Botan::Certificate_Store>> _certstores;
	};

	class BotanSertificateContainer : public ::SertificateContainer
	{
	public:
		void addPem(const std::string& file)
		{
			botanCreds.add(file, file);
		}
		void add(const std::string& crt, const std::string& key)
		{
			botanCreds.add(crt, key);
		}
		ServerCredentials botanCreds;
	};
	

	class BotanFilterCallbacks : public Botan::TLS::Callbacks
	{
	public:
		BotanFilterCallbacks(SessionFilter *filt_, std::deque<char>& pending_output_) :
			filt(filt_), pending_output(pending_output_) {}

	private:
		SessionFilter* filt;
		std::deque<char>& pending_output;

		void tls_emit_data(const uint8_t buf[], size_t length) override
		{
			assert(length < 1000000LU);
			while (length) {
				auto sent = filt->sendFunc((const char*)buf, length);

				if (sent == -1) {
					if (errno == EINTR)
						sent = 0;
					else
						throw Botan::Exception("Socket write failed");
				}

				buf += sent;
				length -= sent;
			}
		}

		void tls_record_received(uint64_t /*seq_no*/, const uint8_t input[], size_t input_len) override
		{
			for (size_t i = 0; i != input_len; ++i) {
				const char c = static_cast<char>(input[i]);
				pending_output.push_back(c);
			}
		}

		void tls_alert(Botan::TLS::Alert alert) override
		{
			std::cout << "Alert: " << alert.type_string() << std::endl;
		}

		bool tls_session_established(const Botan::TLS::Session& session) override
		{
			try
			{
				std::cout << "Handshake complete, " << session.version().to_string()
					<< " using " << session.ciphersuite().to_string() << std::endl;

				if (!session.session_id().empty())
					std::cout << "Session ID " << Botan::hex_encode(session.session_id()) << std::endl;

				if (!session.session_ticket().empty())
					std::cout << "Session ticket " << Botan::hex_encode(session.session_ticket()) << std::endl;

			}
			catch (Botan::Exception& e)
			{
				std::cerr << "botan: " <<
					": Connection failed: " << e.what() << "\n";
			}
			catch (std::exception& e)
			{
				std::cerr << "botan: " << ": " << e.what() << "\n";
				throw;
			}
			return true;
		}

		void tls_verify_cert_chain(const std::vector<Botan::X509_Certificate>&, const std::vector<std::shared_ptr<const Botan::OCSP::Response>>&,
			const std::vector<Botan::Certificate_Store*>&,
			Botan::Usage_Type,
			const std::string&,
			const Botan::TLS::Policy&) override
		{
			// Do not verify sertificate.
		}
	};

	struct BotanData
	{
		Botan::TLS::Channel* server;
		std::deque<char> pending_output;
		std::string peer_fingerprint;
		BotanFilterCallbacks* callbacks;
		Botan::TLS::Policy* policy;
		Botan::System_RNG rng;
		Botan::TLS::Session_Manager_In_Memory* session_manager;
		ServerCredentials* creds;
	};
}


class SessionFilterBotan : public SessionFilter
{
public:
	SessionFilterBotan(SessionFilter::Mode mode, std::shared_ptr<BotanSertificateContainer> sertificates)
	{
		_sertificates = sertificates;
		data.policy = new Botan::TLS::Policy;
		data.session_manager = new Botan::TLS::Session_Manager_In_Memory(data.rng, 10000);
		data.callbacks = new BotanFilterCallbacks(this, data.pending_output);
		data.creds = &_sertificates->botanCreds;
		data.server = nullptr;
		if (mode == SessionFilter::Mode::Server)
		{
			data.server = new Botan::TLS::Server(*data.callbacks,
				*data.session_manager,
				*data.creds,
				*data.policy,
				data.rng);
		}

		memset(receiveBuffer, 0,receiveBufferSize);
	}
	~SessionFilterBotan()
	{
		delete data.callbacks;
		delete data.server;
		delete data.policy;
		delete data.session_manager; // riippuvuus: rng
	}
	unsigned send(const char* buf, unsigned bytes) override
	{
		try
		{
			data.server->send((const uint8_t*)buf, bytes);
			return bytes;
		}
		catch (Botan::Exception&)
		{
		}
		catch (std::exception&)
		{
			throw;
		}
		return 0;
	}
	unsigned receive(char* buffer, unsigned bytes) override
	{
		if (data.pending_output.empty()) {

			try {			
				unsigned desired = Botan::TLS::TLS_HEADER_SIZE;
				while (data.pending_output.empty())
				{
					auto asking = std::max(receiveBufferSize, std::min(desired, 1U));
					int ret = recvFunc((char*)receiveBuffer, asking);
					desired = data.server->received_data(receiveBuffer, ret);
				}
			}
			catch (Botan::Exception&)
			{
			}
			catch (std::exception&)
			{
				throw;
			}
		}
		unsigned bytesReceived = 0;
		for (unsigned i = bytesReceived; i < bytes && data.pending_output.empty() == false; ++i) {
			auto d = data.pending_output.front();
			data.pending_output.pop_front();
			buffer[i] = d;
			bytesReceived++;
		}

		return bytesReceived;
	}
	bool hasData() const override
	{
		return !data.pending_output.empty();
	}
private:
	Botan::byte receiveBuffer[receiveBufferSize];
	BotanData data;
	std::shared_ptr<BotanSertificateContainer> _sertificates;
};

SessionFilterRef peq::network::createFilterTLS(SessionFilter::Mode mode, SertificateContainerRef sertificates)
{
	if (auto s = std::dynamic_pointer_cast<BotanSertificateContainer>(sertificates))
	{
		return SessionFilterRef(new SessionFilterBotan(mode, s));
	}
	else
	{
		return nullptr;
	}
}

SertificateContainerRef peq::network::createSertificateContainer()
{
	return SertificateContainerRef(new BotanSertificateContainer());
}

#endif
