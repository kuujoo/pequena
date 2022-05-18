#include "pequena/network/http/http.h"
#include "pequena/time.h"
#include "pequena/stringutils.h"
#include "pequena/crypto/crypto.h"
#include "pequena/log.h"
#include <sstream>
#include <istream>
#include <filesystem>
#include <regex>
#include <ctime>


using namespace peq;
using namespace peq::http;
using namespace peq::network;

namespace
{
	// Headers
	const std::string s_dateHeader = "Date";
	const std::string s_authorizationHeader = "Authorization";
	const std::string s_connectionHeader = "Connection";
	const std::string s_keepAliveHeader = "Keep-Alive";
	const std::string s_contentTypeHeader = "Content-Type";
	const std::string s_contentLengthHeader = "Content-Length";
	const std::string s_apikeyHeader = "x-api-key";
	const std::string s_cookie = "Cookie";
	const std::string s_setCookieHeader = "Set-Cookie";
	const std::string s_cacheControl = "Cache-Control";
	// Values
	const std::string s_keepAliveValue = "Keep-Alive";
	const std::string s_closeValue = "Close";
	const std::string s_empty = "";

	bool compare(const std::string& a, const std::string& b)
	{
		if (a.size() != b.size()) return false;

		return std::equal(a.begin(), a.end(),
			b.begin(), b.end(),
			[](char a, char b) {
				return tolower(a) == tolower(b);
			});
	}

	void httpDate(char* buf, size_t buf_len, struct tm* tm)
	{
		const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
		const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

		snprintf(buf, buf_len, "%s, %d %s %d %02d:%02d:%02d GMT",
			days[tm->tm_wday], tm->tm_mday, months[tm->tm_mon],
			tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
	}

	std::string httpDate(uint64_t epoch)
	{
		char buf[100] = { 0 };
		time_t t = static_cast<time_t>(epoch);
		struct tm* tm = gmtime(&t);
		httpDate(buf, 100, tm);
		return std::string(buf);
	}

	std::string httpDateNow()
	{
		return httpDate(peq::time::epochS());
	}

	std::string toString(Status code)
	{
		int c = (int)code;
		switch (c)
		{
		case 100: return "Continue";
		case 101: return "Switching Protocols";
		case 102: return "Processing";
		case 103: return "Early Hints";
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";
		case 205: return "Reset Content";
		case 206: return "Partial Content";
		case 207: return "Multi-Status";
		case 208: return "Already Reported";
		case 226: return "IM Used";
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 305: return "Use Proxy";
		case 307: return "Temporary Redirect";
		case 308: return "Permanent Redirect";
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 407: return "Proxy Authentication Required";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 412: return "Precondition Failed";
		case 413: return "Content Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 416: return "Range Not Satisfiable";
		case 417: return "Expectation Failed";
		case 418: return "I'm a teapot";
		case 421: return "Misdirected Request";
		case 422: return "Unprocessable Content";
		case 423: return "Locked";
		case 424: return "Failed Dependency";
		case 425: return "Too Early";
		case 426: return "Upgrade Required";
		case 428: return "Precondition Required";
		case 429: return "Too Many Requests";
		case 431: return "Request Header Fields Too Large";
		case 451: return "Unavailable For Legal Reasons";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";
		case 506: return "Variant Also Negotiates";
		case 507: return "Insufficient Storage";
		case 508: return "Loop Detected";
		case 510: return "Not Extended";
		case 511: return "Network Authentication Required";
		default: return std::string();
		}
	}
	struct MimeType
	{
		std::string extension;
		std::string type;
	};

	MimeType s_mimeTypes[348] = {
		{"*3gpp", "audio/3gpp"},
		{"*jpm", "video/jpm"},
		{"*mp3", "audio/mp3"},
		{"*rtf", "text/rtf"},
		{"*wav", "audio/wave"},
		{"*xml", "text/xml"},
		{"3g2", "video/3gpp2"},
		{"3gp", "video/3gpp"},
		{"3gpp", "video/3gpp"},
		{"ac", "application/pkix-attr-cert"},
		{"adp", "audio/adpcm"},
		{"ai", "application/postscript"},
		{"apng", "image/apng"},
		{"appcache", "text/cache-manifest"},
		{"asc", "application/pgp-signature"},
		{"atom", "application/atom+xml"},
		{"atomcat", "application/atomcat+xml"},
		{"atomsvc", "application/atomsvc+xml"},
		{"au", "audio/basic"},
		{"aw", "application/applixware"},
		{"bdoc", "application/bdoc"},
		{"bin", "application/octet-stream"},
		{"bmp", "image/bmp"},
		{"bpk", "application/octet-stream"},
		{"buffer", "application/octet-stream"},
		{"ccxml", "application/ccxml+xml"},
		{"cdmia", "application/cdmi-capability"},
		{"cdmic", "application/cdmi-container"},
		{"cdmid", "application/cdmi-domain"},
		{"cdmio", "application/cdmi-object"},
		{"cdmiq", "application/cdmi-queue"},
		{"cer", "application/pkix-cert"},
		{"cgm", "image/cgm"},
		{"class", "application/java-vm"},
		{"coffee", "text/coffeescript"},
		{"conf", "text/plain"},
		{"cpt", "application/mac-compactpro"},
		{"crl", "application/pkix-crl"},
		{"css", "text/css"},
		{"csv", "text/csv"},
		{"cu", "application/cu-seeme"},
		{"davmount", "application/davmount+xml"},
		{"dbk", "application/docbook+xml"},
		{"deb", "application/octet-stream"},
		{"def", "text/plain"},
		{"event-stream", "text/event-stream"},
		{"deploy", "application/octet-stream"},
		{"disposition-notification", "message/disposition-notification"},
		{"dist", "application/octet-stream"},
		{"distz", "application/octet-stream"},
		{"dll", "application/octet-stream"},
		{"dmg", "application/octet-stream"},
		{"dms", "application/octet-stream"},
		{"doc", "application/msword"},
		{"dot", "application/msword"},
		{"drle", "image/dicom-rle"},
		{"dssc", "application/dssc+der"},
		{"dtd", "application/xml-dtd"},
		{"dump", "application/octet-stream"},
		{"ear", "application/java-archive"},
		{"ecma", "application/ecmascript"},
		{"elc", "application/octet-stream"},
		{"emf", "image/emf"},
		{"eml", "message/rfc822"},
		{"emma", "application/emma+xml"},
		{"eps", "application/postscript"},
		{"epub", "application/epub+zip"},
		{"es", "application/ecmascript"},
		{"exe", "application/octet-stream"},
		{"exi", "application/exi"},
		{"exr", "image/aces"},
		{"ez", "application/andrew-inset"},
		{"fits", "image/fits"},
		{"g3", "image/g3fax"},
		{"gbr", "application/rpki-ghostbusters"},
		{"geojson", "application/geo+json"},
		{"gif", "image/gif"},
		{"glb", "model/gltf-binary"},
		{"gltf", "model/gltf+json"},
		{"gml", "application/gml+xml"},
		{"gpx", "application/gpx+xml"},
		{"gram", "application/srgs"},
		{"grxml", "application/srgs+xml"},
		{"gxf", "application/gxf"},
		{"gz", "application/gzip"},
		{"h261", "video/h261"},
		{"h263", "video/h263"},
		{"h264", "video/h264"},
		{"heic", "image/heic"},
		{"heics", "image/heic-sequence"},
		{"heif", "image/heif"},
		{"heifs", "image/heif-sequence"},
		{"hjson", "application/hjson"},
		{"hlp", "application/winhlp"},
		{"hqx", "application/mac-binhex40"},
		{"htm", "text/html"},
		{"html", "text/html"},
		{"ics", "text/calendar"},
		{"ief", "image/ief"},
		{"ifb", "text/calendar"},
		{"iges", "model/iges"},
		{"igs", "model/iges"},
		{"img", "application/octet-stream"},
		{"in", "text/plain"},
		{"ini", "text/plain"},
		{"ink", "application/inkml+xml"},
		{"inkml", "application/inkml+xml"},
		{"ipfix", "application/ipfix"},
		{"iso", "application/octet-stream"},
		{"jade", "text/jade"},
		{"jar", "application/java-archive"},
		{"jls", "image/jls"},
		{"jp2", "image/jp2"},
		{"jpe", "image/jpeg"},
		{"jpeg", "image/jpeg"},
		{"jpf", "image/jpx"},
		{"jpg", "image/jpeg"},
		{"jpg2", "image/jp2"},
		{"jpgm", "video/jpm"},
		{"jpgv", "video/jpeg"},
		{"jpm", "image/jpm"},
		{"jpx", "image/jpx"},
		{"js", "application/javascript"},
		{"json", "application/json"},
		{"json5", "application/json5"},
		{"jsonld", "application/ld+json"},
		{"jsonml", "application/jsonml+json"},
		{"jsx", "text/jsx"},
		{"kar", "audio/midi"},
		{"ktx", "image/ktx"},
		{"less", "text/less"},
		{"list", "text/plain"},
		{"litcoffee", "text/coffeescript"},
		{"log", "text/plain"},
		{"lostxml", "application/lost+xml"},
		{"lrf", "application/octet-stream"},
		{"m1v", "video/mpeg"},
		{"m21", "application/mp21"},
		{"m2a", "audio/mpeg"},
		{"m2v", "video/mpeg"},
		{"m3a", "audio/mpeg"},
		{"m4a", "audio/mp4"},
		{"m4p", "application/mp4"},
		{"ma", "application/mathematica"},
		{"mads", "application/mads+xml"},
		{"man", "text/troff"},
		{"manifest", "text/cache-manifest"},
		{"map", "application/json"},
		{"mar", "application/octet-stream"},
		{"markdown", "text/markdown"},
		{"mathml", "application/mathml+xml"},
		{"mb", "application/mathematica"},
		{"mbox", "application/mbox"},
		{"md", "text/markdown"},
		{"me", "text/troff"},
		{"mesh", "model/mesh"},
		{"meta4", "application/metalink4+xml"},
		{"metalink", "application/metalink+xml"},
		{"mets", "application/mets+xml"},
		{"mft", "application/rpki-manifest"},
		{"mid", "audio/midi"},
		{"midi", "audio/midi"},
		{"mime", "message/rfc822"},
		{"mj2", "video/mj2"},
		{"mjp2", "video/mj2"},
		{"mjs", "application/javascript"},
		{"mml", "text/mathml"},
		{"mods", "application/mods+xml"},
		{"mov", "video/quicktime"},
		{"mp2", "audio/mpeg"},
		{"mp21", "application/mp21"},
		{"mp2a", "audio/mpeg"},
		{"mp3", "audio/mpeg"},
		{"mp4", "video/mp4"},
		{"mp4a", "audio/mp4"},
		{"mp4s", "application/mp4"},
		{"mp4v", "video/mp4"},
		{"mpd", "application/dash+xml"},
		{"mpe", "video/mpeg"},
		{"mpeg", "video/mpeg"},
		{"mpg", "video/mpeg"},
		{"mpg4", "video/mp4"},
		{"mpga", "audio/mpeg"},
		{"mrc", "application/marc"},
		{"mrcx", "application/marcxml+xml"},
		{"ms", "text/troff"},
		{"mscml", "application/mediaservercontrol+xml"},
		{"msh", "model/mesh"},
		{"msi", "application/octet-stream"},
		{"msm", "application/octet-stream"},
		{"msp", "application/octet-stream"},
		{"mxf", "application/mxf"},
		{"mxml", "application/xv+xml"},
		{"n3", "text/n3"},
		{"nb", "application/mathematica"},
		{"oda", "application/oda"},
		{"oga", "audio/ogg"},
		{"ogg", "audio/ogg"},
		{"ogv", "video/ogg"},
		{"ogx", "application/ogg"},
		{"omdoc", "application/omdoc+xml"},
		{"onepkg", "application/onenote"},
		{"onetmp", "application/onenote"},
		{"onetoc", "application/onenote"},
		{"onetoc2", "application/onenote"},
		{"opf", "application/oebps-package+xml"},
		{"otf", "font/otf"},
		{"owl", "application/rdf+xml"},
		{"oxps", "application/oxps"},
		{"p10", "application/pkcs10"},
		{"p7c", "application/pkcs7-mime"},
		{"p7m", "application/pkcs7-mime"},
		{"p7s", "application/pkcs7-signature"},
		{"p8", "application/pkcs8"},
		{"pdf", "application/pdf"},
		{"pfr", "application/font-tdpfr"},
		{"pgp", "application/pgp-encrypted"},
		{"pkg", "application/octet-stream"},
		{"pki", "application/pkixcmp"},
		{"pkipath", "application/pkix-pkipath"},
		{"pls", "application/pls+xml"},
		{"png", "image/png"},
		{"prf", "application/pics-rules"},
		{"ps", "application/postscript"},
		{"pskcxml", "application/pskc+xml"},
		{"qt", "video/quicktime"},
		{"raml", "application/raml+yaml"},
		{"rdf", "application/rdf+xml"},
		{"rif", "application/reginfo+xml"},
		{"rl", "application/resource-lists+xml"},
		{"rld", "application/resource-lists-diff+xml"},
		{"rmi", "audio/midi"},
		{"rnc", "application/relax-ng-compact-syntax"},
		{"rng", "application/xml"},
		{"roa", "application/rpki-roa"},
		{"roff", "text/troff"},
		{"rq", "application/sparql-query"},
		{"rs", "application/rls-services+xml"},
		{"rsd", "application/rsd+xml"},
		{"rss", "application/rss+xml"},
		{"rtf", "application/rtf"},
		{"rtx", "text/richtext"},
		{"s3m", "audio/s3m"},
		{"sbml", "application/sbml+xml"},
		{"scq", "application/scvp-cv-request"},
		{"scs", "application/scvp-cv-response"},
		{"sdp", "application/sdp"},
		{"ser", "application/java-serialized-object"},
		{"setpay", "application/set-payment-initiation"},
		{"setreg", "application/set-registration-initiation"},
		{"sgi", "image/sgi"},
		{"sgm", "text/sgml"},
		{"sgml", "text/sgml"},
		{"shex", "text/shex"},
		{"shf", "application/shf+xml"},
		{"shtml", "text/html"},
		{"sig", "application/pgp-signature"},
		{"sil", "audio/silk"},
		{"silo", "model/mesh"},
		{"slim", "text/slim"},
		{"slm", "text/slim"},
		{"smi", "application/smil+xml"},
		{"smil", "application/smil+xml"},
		{"snd", "audio/basic"},
		{"so", "application/octet-stream"},
		{"spp", "application/scvp-vp-response"},
		{"spq", "application/scvp-vp-request"},
		{"spx", "audio/ogg"},
		{"sru", "application/sru+xml"},
		{"srx", "application/sparql-results+xml"},
		{"ssdl", "application/ssdl+xml"},
		{"ssml", "application/ssml+xml"},
		{"stk", "application/hyperstudio"},
		{"styl", "text/stylus"},
		{"stylus", "text/stylus"},
		{"svg", "image/svg+xml"},
		{"svgz", "image/svg+xml"},
		{"t", "text/troff"},
		{"t38", "image/t38"},
		{"tei", "application/tei+xml"},
		{"teicorpus", "application/tei+xml"},
		{"text", "text/plain"},
		{"tfi", "application/thraud+xml"},
		{"tfx", "image/tiff-fx"},
		{"tif", "image/tiff"},
		{"tiff", "image/tiff"},
		{"tr", "text/troff"},
		{"ts", "video/mp2t"},
		{"tsd", "application/timestamped-data"},
		{"tsv", "text/tab-separated-values"},
		{"ttc", "font/collection"},
		{"ttf", "font/ttf"},
		{"ttl", "text/turtle"},
		{"txt", "text/plain"},
		{"u8dsn", "message/global-delivery-status"},
		{"u8hdr", "message/global-headers"},
		{"u8mdn", "message/global-disposition-notification"},
		{"u8msg", "message/global"},
		{"uri", "text/uri-list"},
		{"uris", "text/uri-list"},
		{"urls", "text/uri-list"},
		{"vcard", "text/vcard"},
		{"vrml", "model/vrml"},
		{"vtt", "text/vtt"},
		{"vxml", "application/voicexml+xml"},
		{"war", "application/java-archive"},
		{"wasm", "application/wasm"},
		{"wav", "audio/wav"},
		{"weba", "audio/webm"},
		{"webm", "video/webm"},
		{"webmanifest", "application/manifest+json"},
		{"webp", "image/webp"},
		{"wgt", "application/widget"},
		{"wmf", "image/wmf"},
		{"woff", "font/woff"},
		{"woff2", "font/woff2"},
		{"wrl", "model/vrml"},
		{"wsdl", "application/wsdl+xml"},
		{"wspolicy", "application/wspolicy+xml"},
		{"x3d", "model/x3d+xml"},
		{"x3db", "model/x3d+binary"},
		{"x3dbz", "model/x3d+binary"},
		{"x3dv", "model/x3d+vrml"},
		{"x3dvz", "model/x3d+vrml"},
		{"x3dz", "model/x3d+xml"},
		{"xaml", "application/xaml+xml"},
		{"xdf", "application/xcap-diff+xml"},
		{"xdssc", "application/dssc+xml"},
		{"xenc", "application/xenc+xml"},
		{"xer", "application/patch-ops-error+xml"},
		{"xht", "application/xhtml+xml"},
		{"xhtml", "application/xhtml+xml"},
		{"xhvml", "application/xv+xml"},
		{"xm", "audio/xm"},
		{"xml", "application/xml"},
		{"xop", "application/xop+xml"},
		{"xpl", "application/xproc+xml"},
		{"xsd", "application/xml"},
		{"xsl", "application/xml"},
		{"xslt", "application/xslt+xml"},
		{"xspf", "application/xspf+xml"},
		{"xvm", "application/xv+xml"},
		{"xvml", "application/xv+xml"},
		{"yaml", "text/yaml"},
		{"yang", "application/yang"},
		{"yin", "application/yin+xml"},
		{"yml", "text/yaml"},
		{"zip", "application/zip"},
	};

	std::string mimeTypeByExtension(const std::string& ext)
	{
		auto m = std::find_if(std::begin(s_mimeTypes), std::end(s_mimeTypes), [&ext](const MimeType& mime)->bool {
			return ext == mime.extension;
		});

		if (m == std::end(s_mimeTypes))
		{
			return "text/plain";
		}

		return m->type;
	}
	std::string getMimeType(const std::string& file)
	{
		auto dot = file.find_last_of('.');
		if (dot == std::string::npos)
		{
			return mimeTypeByExtension(file);
		}
		else
		{
			return mimeTypeByExtension(file.substr(dot + 1));
		}
	}
	http::Data toData(const http::Response& response)
	{
		std::stringstream ss;
		ss << "HTTP/" << response.version.major << "." << response.version.minor << " " << (int)response.status << " " << toString(response.status) << "\r\n";
		for (auto& it : response.headers)
		{
			ss << it.name << ": " << it.value << "\r\n";
		}
		ss << "\r\n";

		auto str = ss.str();

		http::Data data;
		data.resize(str.size() + response.body.size());
		memcpy(data.data(), str.c_str(), str.length());
		memcpy(data.data() + str.size(), response.body.data(), response.body.size());

		return data;
	}
}

Header Header::createKeepAliveResponse(unsigned seconds, unsigned requests)
{
	std::stringstream st;
	st << "timeout=" << seconds << ", max=" << requests;
	return Header(s_keepAliveHeader, st.str());
}

Header Header::createContentTypeResponse(const std::string& type)
{
	return Header(s_contentTypeHeader, getMimeType(type) );
}

Cookie Cookie::create(const std::string& cookie)
{
	auto p = cookie.find('=');
	if (p == std::string::npos) return Cookie();

	auto key = cookie.substr(0, p);
	auto value = cookie.substr(p + 1);

	return Cookie(key, value);
}

Cookie::Cookie(const std::string& name, const std::string& value) :
	name(name), value(value)
{
}

Authorization::Authorization()
{

}

Authorization::Authorization(const std::string& authenticationHeaderValue)
{
	const char basic[] = { 'B','a','s','i','c',' ' };
	const char bearer[] = { 'B','e','a','r','e','r', ' ' };
	if (memcmp(authenticationHeaderValue.c_str(), basic, sizeof(basic)) == 0)
	{
		type = Authorization::Type::Basic;
		auto decoded = peq::crypto::base64Decode(std::string(authenticationHeaderValue.c_str() + sizeof(basic), authenticationHeaderValue.size() - sizeof(basic)));
		if (!decoded.empty())
		{
			std::string str(decoded.size(), '\0');
			memcpy(str.data(), decoded.data(), decoded.size());
			auto colonLoc = str.find(':', 0);
			user = str.substr(0, colonLoc);
			password = str.substr(colonLoc + 1);
		}
	}
	else if (memcmp(authenticationHeaderValue.c_str(), bearer, sizeof(bearer)) == 0)
	{
		user = std::string(authenticationHeaderValue.c_str() + sizeof(bearer), authenticationHeaderValue.size() - sizeof(bearer));
		type = Authorization::Type::Bearer;
	}
}

bool Authorization::empty() const
{
	return user.empty() && password.empty();
}

const Authorization Request::auth() const
{
	for (auto& h : headers)
	{
		if (h.name == s_authorizationHeader)
		{
			return Authorization(h.value);
		}
	}
	return Authorization();
}

const Apikey Request::apiKey() const
{
	for (auto& h : headers)
	{
		if (h.name == s_apikeyHeader)
		{
			return Apikey(h.value);
		}
	}
	return Apikey();
}

const http::Cookie Request::cookie(const std::string& cookie) const
{
	for (auto & h : headers)
	{
		if (h.name == s_cookie)
		{
			if (cookie.size() > h.value.size()) continue;
		
			std::istringstream st(h.value);
			std::string tmp;
			while (std::getline(st, tmp, ';'))
			{
				if (tmp.empty())
				{
					continue;
				}

				auto offset = 0;
				if (tmp[0] == ' ')
				{
					offset = 1;
				}
				
				auto equalp = tmp.find('=');
				if (equalp == std::string::npos) continue;
				if (equalp <= offset) continue;

				auto sub = tmp.substr(offset, equalp - offset);
				if (sub == cookie)
				{
					return Cookie::create(tmp.c_str() + offset);
				}
			}
		}
	}
	return http::Cookie();
}

bool Request::wantsToKeepAlive() const
{
	for (auto& h : headers)
	{
		if (compare(h.name, s_connectionHeader) &&  compare(h.value, s_keepAliveValue))
		{
			return true;
		}
	}
	return false;
}

bool Request::wantsToClose() const
{
	for (auto& h : headers)
	{
		if (compare(h.name, s_connectionHeader) && compare(h.value, s_closeValue))
		{
			return true;
		}
	}
	return false;
}

Response Response::createText(Status status, const std::string& content)
{
	http::Data d;
	d.resize(content.size());
	memcpy(d.data(), content.c_str(), content.size());
	return http::Response::create(status, "text", std::move(d));
}

Response Response::createEventStream(Status status, const std::string& content)
{
	http::Data d;
	d.resize(content.size());
	memcpy(d.data(), content.c_str(), content.size());
	auto resp = http::Response::create(status, "event-stream", std::move(d));
	resp.headers.push_back(peq::http::Header(s_cacheControl, "no-cache"));
	return resp;
}

Response Response::createJson(Status status, const std::string& content)
{
	http::Data d;
	d.resize(content.size());
	memcpy(d.data(), content.c_str(), content.size());
	return http::Response::create(status, "json", std::move(d));
}

Response Response::createFromFilename(Status status, const std::string& filename, http::Data &&content)
{
	std::filesystem::path filePath = std::filesystem::u8path(filename.data());
	auto e = filePath.extension().u8string();
	return http::Response::create(status, e, std::forward<http::Data>(content));
}

Response Response::create(Status status, const std::string& type, http::Data&& content)
{
	Response r(status, std::forward<http::Data>(content));
	r.headers.push_back(Header::createContentTypeResponse(type));
	return r;
}

Response::Response(Status stat, http::Data&& content)
{
	http::Response resp;
	version.minor = 1;
	version.major = 1;
	status = stat;
	headers.push_back(http::Header(s_dateHeader, httpDateNow()));
	headers.push_back(http::Header(s_contentLengthHeader, content.size()));
	body = content;
}

Response::Response(Status stat)
{
	http::Response resp;
	version.minor = 1;
	version.major = 1;
	status = stat;
	headers.push_back(http::Header(s_dateHeader, httpDateNow()));
	headers.push_back(http::Header(s_contentLengthHeader, 0));
}

void Response::setCookie(const Cookie& cookie)
{
	std::string value = cookie.name;
	value.append("=");
	value.append(cookie.value);

	if (cookie.sameSite == Cookie::SameSite::Lax)
	{
		value += "; SameSite=Lax";
	}
	else if (cookie.sameSite == Cookie::SameSite::Strict)
	{
		value += "; SameSite=Strict";
	}
	else if (cookie.sameSite == Cookie::SameSite::None)
	{
		value += "; SameSite=None";
	}

	if (cookie.maxAge != 0)
	{
		value += "; Max-Age=" + peq::string::from(cookie.maxAge);
	}

	if (cookie.secure)
	{
		value += "; Secure";
	}

	if (cookie.httpOnly)
	{
		value += "; HttpOnly";
	}

	if (!cookie.path.empty())
	{
		value += "; Path=" + cookie.path;
	}
	
	headers.push_back(http::Header(s_setCookieHeader, value));
}


Router& Router::set(Method method, const std::string& route,
	std::function<Response(const Request&)> func,
	std::function<bool(const http::Request&)> authFunc)
{
	Url url(route);

	Handle handle;
	handle.method = method;
	handle.func = func;
	handle.authFunc = authFunc;
	handle.path = url.path;
	auto keys = url.params.keys();
	for (auto& k : keys)
	{
		handle.params.push_back(k);
	}

	std::sort(handle.params.begin(), handle.params.end());

	for (auto &h : _handles)
	{	
		if (handle.method == h.method && handle.path == h.path)
		{
			bool differentparams = false;
			if (h.params.size() == handle.params.size() &&
				std::equal(h.params.begin(), h.params.end(), handle.params.begin()))
			{
				assert(0 && "handler already exists");
				return *this;
			}
		}
	}

	_handles.push_back(std::move(handle));
	return *this;
}

Parameters::Parameters(const std::string& url)
{
	//aaaa?b=4&d=4
	auto start = url.find_first_of('?');
	std::vector<std::string> keyvalues;
	if (start == 0 || start >= url.size() - 1) return;

	std::istringstream sstream(url.substr(start + 1));
	std::string str;
	while (std::getline(sstream, str, '&'))
	{
		auto equal = str.find_first_of('=');
		if (equal == 0 || equal >= str.size() - 1)
		{
			// no equal sign ?
			// set parameter with empty value and move on
			set(str, std::string());
			continue;
		}

		auto key = str.substr(0, equal);
		auto value = str.substr(equal + 1);
		set(key, value);
	}
}

const std::string& Parameters::get(const std::string& key) const
{
	auto it = _params.find(key);
	if (it == _params.end()) return s_empty;
	return it->second;
}

std::string Parameters::s(const std::string& key) const
{
	const auto& s = get(key);
	return s;
}

int Parameters::i(const std::string& key) const
{
	const auto& s = get(key);
	if (s.empty())
	{
		return std::numeric_limits<int>::min();
	}

	return peq::string::to<int>(s);
}

int64_t Parameters::i64(const std::string& key) const
{
	const auto& s = get(key);
	if (s.empty())
	{
		return std::numeric_limits<int64_t>::min();
	}
	return peq::string::to<int64_t>(s);
}

bool Parameters::b(const std::string& key) const
{
	const auto& s = get(key);
	if (s == "true") return true;
	if (s == "false") return false;
	if (s == "1") return true;
	if (s == "0") return false;
	return false;
}

void Parameters::set(const std::string& key, const std::string& value)
{
	_params[key] = value;
}

std::vector<std::string> Parameters::keys() const
{
	std::vector<std::string> p;
	for (auto it : _params)
	{
		p.push_back(it.first);
	}
	return p;
}

Url::Url(const std::string& fullUrl)
{
	full = fullUrl;
	auto start = fullUrl.find('?');
	if (start == std::string::npos)
	{
		path = fullUrl;
	}
	else
	{
		path = fullUrl.substr(0, start);
		params = Parameters(fullUrl);
	}
}

Router& Router::setDefault(std::function<Response(const Request&)> func)
{
	_defaultFunc = func;
	return *this;
}

Response Router::route(const Request& request)
{
	auto endpointFound = false;
	for (auto& it : _handles)
	{
		auto match = std::regex_match(request.url.path, std::regex(it.path));
		if (match) endpointFound = true;

		if (it.method == request.method && match)
		{
			bool found = true;
			auto requestParamKeys = request.url.params.keys();
			if (requestParamKeys.size() != it.params.size()) continue;

			std::sort(requestParamKeys.begin(), requestParamKeys.end());
			if (std::equal(requestParamKeys.begin(), requestParamKeys.end(), it.params.begin()))
			{
				if (it.authFunc)
				{
					if (!it.authFunc(request)) {
						return Response::createText(peq::http::Status::Unauthorized, "");
					}
				}
				return it.func(request);
			}	
		}
	}

	if (_defaultFunc != nullptr)
	{
		return _defaultFunc(request);
	}

	if (endpointFound)
	{
		return Response::createText(peq::http::Status::BadRequest, "Action not found");
	}
	else
	{
		return Response::createText(peq::http::Status::NotFound, "Resource not found");
	}
}

HttpSession::HttpSession() : Session(), _keepAlive(false), _persistent(false), _keepAliveTimeout(15), _keepAlivemMaxRequests(1000), _timeout(10), _requests(0)
{
	llhttp_settings_init(&_settings);
	_settings.on_message_complete = &handleOnMessageComplete;
	_settings.on_headers_complete = &handleOnHeadersComplete;
	_settings.on_header_field = &handleOnHeaderField;
	_settings.on_header_value = &handleOnHeaderValue;
	_settings.on_header_value_complete = &handleOnHeaderValueComplete;
	_settings.on_header_field_complete = &handleOnHeaderFieldComplete;
	_settings.on_url = &handleOnUrl;
	_settings.on_body = &handleOnBody;
	llhttp_init(&_parser, HTTP_BOTH, &_settings);
	_parser.data = this;

	peq::log::debug("http session created");
}

HttpSession::~HttpSession()
{
	peq::log::debug("http session destroyed");
}

void HttpSession::dataAvailable()
{
	auto received = receive(_buffer, peq::network::receiveBufferSize);
	if (received > 0)
	{
		auto err = llhttp_execute(&_parser, _buffer, received);
		if (err == HPE_OK)
		{
			if (_parseState.complete)
			{
				peq::log::debug("http request received");
				_currentRequest = http::Request();
				_currentRequest.method = _parseState.method;
				_currentRequest.url = std::move(_parseState.url);
				_currentRequest.headers = std::move(_parseState.headers);

				_currentRequest.body.resize(_parseState.bodyLength);
				memcpy(_currentRequest.body.data(), _parseState.bodyStart, _parseState.bodyLength);
				_currentRequest.version = std::move(_parseState.version);
				_currentRequest.info = info();
				_currentRequest.secure = secure();
				_requests++;

				if (_currentRequest.wantsToKeepAlive())
				{
					_keepAlive = true;
				}

				_parseState = ParseState();
				_idleStarted = peq::time::epochS();

				httpRequestAvailable(_currentRequest);

				if (!_currentRequest.wantsToKeepAlive())
				{
					disconnect();
				}
			}
		}
	}
	else
	{
		disconnect();
	}
}

void HttpSession::makePersistent()
{
	_persistent = true;
}

int HttpSession::send(http::Response& response)
{
	if (response.headers.empty() && response.body.empty()) return 0;

	_idleStarted = peq::time::epochS();

	if (_currentRequest.wantsToKeepAlive())
	{
		response.headers.push_back(Header::createKeepAliveResponse(_keepAliveTimeout, _keepAlivemMaxRequests));
	}
	auto d = toData(response);

	peq::log::debug("http response sent");

	return Session::send(d.data(), d.size());
}

void HttpSession::update()
{
	if (_persistent)
	{
		return;
	}

	if (_keepAlive)
	{
		if (peq::time::epochS() - _idleStarted > static_cast<uint64_t>(_keepAliveTimeout) || _requests > _keepAlivemMaxRequests)
		{
			peq::log::debug("disconnect http session (keep-alive timeout)");
			disconnect();
		}
	}
	else
	{
		if (_requests == 0) 
		{
			// Browsers like to open connections for better responsiviness.
			// Close these temporary connections after _timeout seconds
			if (peq::time::epochS() - _idleStarted > static_cast<uint64_t>(_timeout)) 		
			{
				peq::log::debug("disconnect http session (timeout)");
				disconnect();
			}
		}
	}
}

void HttpSession::setKeepaliveTimeout(unsigned seconds)
{
	_keepAliveTimeout = seconds;
}

void HttpSession::setKeepaliveMaxRequests(unsigned maxRequests) 
{
	_keepAlivemMaxRequests = maxRequests;
}

void HttpSession::setTimeout(unsigned seconds)
{
	_timeout = seconds;
}

int HttpSession::handleOnHeaderField(llhttp_t* h, const char* at, size_t length)
{
	auto session = (HttpSession*)h->data;
	session->_parseState.currentHeader.name += std::string(at, length);
	return 0;
}

int HttpSession::handleOnHeaderValue(llhttp_t* h, const char* at, size_t length)
{
	auto session = (HttpSession*)h->data;
	session->_parseState.currentHeader.value += std::string(at, length);
	return 0;
}

int HttpSession::handleOnMessageComplete(llhttp_t* h)
{
	auto session = (HttpSession*)h->data;
	session->_parseState.complete = true;
	session->_parseState.method = (http::Method)h->method;
	session->_parseState.version.minor = h->http_minor;
	session->_parseState.version.major = h->http_major;
	return 0;
}

int HttpSession::handleOnHeadersComplete(llhttp_t* h)
{
	auto session = (HttpSession*)h->data;
	return 0;
}

int HttpSession::handleOnHeaderValueComplete(llhttp_t* h)
{
	auto session = (HttpSession*)h->data;
	session->_parseState.headers.push_back(session->_parseState.currentHeader);
	session->_parseState.currentHeader = http::Header();
	return 0;
}

int HttpSession::handleOnHeaderFieldComplete(llhttp_t* h)
{
	return 0;
}

int HttpSession::handleOnBody(llhttp_t* h, const char* at, size_t length)
{
	auto session = (HttpSession*)h->data;
	if (session->_parseState.bodyStart == nullptr)
	{
		session->_parseState.bodyStart = at;
	}
	session->_parseState.bodyLength += length;
	return 0;
}

int HttpSession::handleOnUrl(llhttp_t* h, const char* at, size_t length)
{
	auto session = (HttpSession*)h->data;
	session->_parseState.url = Url(std::string(at, length));
	return 0;
}

