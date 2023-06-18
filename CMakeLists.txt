cmake_minimum_required(VERSION 3.14)

project(gcutils LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(gcutils SHARED
  gcutils_global.h
  gcutils.cpp
  gcutils.h
  error.h
  error.cpp
)

#set_target_properties(gcutils PROPERTIES SOVERSION 1)

#set_target_properties(gcutils PROPERTIES PUBLIC_HEADER gcutils.h)

target_link_libraries(gcutils PRIVATE stdc++fs)

target_compile_definitions(gcutils PRIVATE GCUTILS_LIBRARY)
