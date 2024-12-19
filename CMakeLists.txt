cmake_minimum_required(VERSION 3.5)

project(server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable( can_logger
  can_logger.cpp
  DDS/LogEntryPubSubTypes.cxx
  DDS/LogEntryTypeObjectSupport.cxx
)

target_link_libraries( can_logger
  fastdds
  fastcdr
  sqlite3
)

target_include_directories( can_logger PUBLIC
  ~/Fast-DDS/install/include
)

target_link_directories( can_logger PUBLIC
  ~/Fast-DDS/install/lib
)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wstrict-aliasing -pedantic")

add_executable(ecu_mock ecu_mock.cpp)
add_executable(scales_mock scales_mock.cpp)
