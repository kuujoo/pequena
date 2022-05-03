cmake_minimum_required(VERSION 3.12)
project(pequena)

# C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE)

add_library(pequena
	"src/log.cpp"
	"src/services.cpp"
	"src/time.cpp"
	$<$<PLATFORM_ID:Windows>:
	"src/network/network_backend_winsock.cpp"
	"src/platform/platform_backend_windows.cpp"
	>
	$<$<PLATFORM_ID:Linux>:
	"src/network/network_backend_bsd.cpp"
	>
	"src/network/network.cpp"
	"src/network/network_backend_botan.cpp"
	"src/network/network_backend_nobotan.cpp"
	"src/network/http/http.cpp"
	"src/network/http/files.cpp"
	"src/network/http/jwt.cpp"
	"src/database/sqlite.cpp"
	"src/crypto/crypto.cpp"
	"src/crypto/crypto_botan.cpp"
	"src/crypto/crypto_nobotan.cpp"
# included Library sources
	"libs/base64/base64.cpp"
	"libs/lhttp/api.c"
	"libs/lhttp/http.c"
	"libs/lhttp/llhttp.c"
	"libs/sqlite/sqlite3.c"
	"libs/hmac_sha256/hmac_sha256.c"
	"libs/hmac_sha256/sha256.c")
		
 # included library headers
target_include_directories(pequena PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libs")
target_include_directories(pequena PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libs/hmac_sha256")
target_include_directories(pequena PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libs/uuid")
target_include_directories(pequena PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libs/gsl")

target_include_directories(pequena
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

set(LIBS "")


#
# To use botan: 
# Build dynamic library (.dll)
#
# DEBUG:
# Copy header files to: libs/botan/Debug-x64/include/botan-2
# Copy .dll tolibs/botan/Debug-x64/lib/botand.dll
#
# RELEASE
# Copy header files to: libs/botan/Release-x64/include/botan-2
# Copy .dll tolibs/botan/Release-x64/lib/botand.dll
#
# set(PEQ_BOTAN_INCLUDE botan_include_path)
# set(PEQ_BOTAN_LIB botan_library_path)
# 
#	example: set(PEQ_BOTAN_INCLUDE D:/dev/botan2/Debug-x64/include/botan-2 )
#	example: set(PEQ_BOTAN_LIB D:/dev/botan2/Debug-x64/lib )
#
# debug build requires: botand.dll and botand.lib
# release build requires: botan.dll botan.lib


if (PEQ_BOTAN_INCLUDE AND PEQ_BOTAN_LIB)
	
	target_include_directories(pequena PUBLIC "${PEQ_BOTAN_INCLUDE}")
	target_compile_definitions(pequena PUBLIC PEQ_BOTAN)
#	target_include_directories(pequena PUBLIC PEQ_BOTAN_INCLUDE)
	if(CMAKE_BUILD_TYPE MATCHES Debug)
		set(LIBS ${LIBS} "${PEQ_BOTAN_LIB}/botand.lib")
	else()
		set(LIBS ${LIBS} "${PEQ_BOTAN_LIB}/botan.lib")
	endif()
endif()

target_link_libraries(pequena PUBLIC ${LIBS})
