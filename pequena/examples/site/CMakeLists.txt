cmake_minimum_required(VERSION 3.12)
project(site)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(site
	src/main.cpp
)

target_include_directories(site PUBLIC  ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(site pequena)

add_custom_command(
	TARGET site POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/content" "${CMAKE_CURRENT_BINARY_DIR}/content"
	COMMENT "copy content"
)