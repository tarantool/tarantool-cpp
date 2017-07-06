cmake_minimum_required (VERSION 2.6)
project(tarantool-cpp)

cmake_policy(SET CMP0015 NEW)


set ("${PROJECT}_BINARY_DIR" bin)
set ("${PROJECT}_SOURCE_DIR" src:include)
set ("${PROJECT}_LIB_DIR" lib)

set (CMAKE_INCLUDE_PATH ${${PROJECT}_SOURCE_DIR} /opt/scylladb/include/ /usr/local/include/msgpack/)
set (CMAKE_LIBRARY_PATH ${${PROJECT}_LIB_DIR} /opt/scylladb/lib64 /usr/local/lib)
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/${${PROJECT}_BINARY_DIR})

add_definitions(-DBOOST_COROUTINE_NO_DEPRECATION_WARNING -DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
add_definitions(-std=c++11 -g -Wall -fpermissive -isystem/usr/include/tarantool/)

include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})

add_library(tarantool-cpp SHARED
            src/tarantool_connector.cpp)

add_library(tarantool-cpp-static
            src/tarantool_connector.cpp)

set_target_properties(tarantool-cpp-static PROPERTIES OUTPUT_NAME tarantool-cpp)

install(TARGETS tarantool-cpp-static tarantool-cpp DESTINATION /usr/lib64)
install(DIRECTORY src/ DESTINATION /usr/include/tarantool-cpp
        FILES_MATCHING PATTERN "*.h")

