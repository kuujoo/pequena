cmake_minimum_required(VERSION 3.12)
project(sql)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sql
	src/main.cpp
)

target_include_directories(sql PUBLIC  ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(sql pequena)
