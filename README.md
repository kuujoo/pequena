# Pequena
Small library for service style applications.

# Requirements
- C++ 17
- CMake

## Features
- Cross platform
- Multithreaded server with TLS support
- sqlite database
- Json serialization / deserialization

## Dependencies
Most of the dependencies are included in source format.

### Included
- [nlohmann Json](https://github.com/nlohmann/json) for json support
- [llhttp](https://github.com/nodejs/llhttp) for http parsing
- [cpp-base64](https://github.com/ReneNyffenegger/cpp-base64) for base64 encoding / decoding
- [sqlite](https://github.com/sqlite/sqlite) for databases
- [hmac_sha256](https://github.com/h5p9sl/hmac_sha256) for hashes
- [std::uuid](https://github.com/mariusbancila/stduuid) for uuids
- [GSL](https://github.com/microsoft/GSL) for std::span<> (uuid library requires this)

### Not included
- [botan](https://github.com/randombit/botan) for crypto
