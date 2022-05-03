cmake_minimum_required(VERSION 3.12)
project(server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(api
	src/main.cpp
)

target_include_directories(api PUBLIC  ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(api pequena)
