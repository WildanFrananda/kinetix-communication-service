cmake_minimum_required(VERSION 3.25)

project(kinetix-communication-service VERSION 0.1.0 LANGUAGES CXX)

# C++23, not 20, for one reason: std::expected. Every fallible operation in this service returns
# one, and the alternative — an out-parameter, an exception, or a hand-rolled Result — either
# hides the failure or reinvents what the standard library now ships. Verified available on Apple
# clang with libc++ and on GCC 13, which are the two compilers this repository is built with.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(COMMUNICATION_BUILD_TESTS "Build the unit tests" ON)

# The adapters are off by default, and that is a deliberate split rather than a half-finished
# build. domain/ and app/ hold every rule this service has and depend on nothing but the standard
# library, so they compile and their tests run in seconds on any machine. The adapters pull in
# Drogon, gRPC, Boost and an MQTT client through vcpkg, which is a build measured in hours and
# gigabytes. Keeping the two apart means the layer that can actually be wrong is also the layer
# that is cheap to check.
option(COMMUNICATION_BUILD_ADAPTERS "Build the Drogon, gRPC and MQTT adapters (needs vcpkg)" OFF)

option(COMMUNICATION_WERROR "Treat warnings as errors (on in CI)" OFF)

add_library(communication_warnings INTERFACE)
target_compile_options(communication_warnings INTERFACE
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
  -Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion
  $<$<BOOL:${COMMUNICATION_WERROR}>:-Werror>)


# ---------------------------------------------------------------------------------------------
# domain: the rules. Header-only, and dependent on nothing.
#
# It is an INTERFACE library rather than a static one because there is nothing to compile: every
# value type is small enough that its body belongs in the header, and a domain with no .cpp files
# is a domain with nowhere to hide a dependency.
# ---------------------------------------------------------------------------------------------
add_library(communication_domain INTERFACE)
target_include_directories(communication_domain INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(communication_domain INTERFACE communication_warnings)

# ---------------------------------------------------------------------------------------------
# app: the use cases. Depends on domain and on the port headers, never on an adapter.
# ---------------------------------------------------------------------------------------------
add_library(communication_app STATIC
  src/app/OpenConversation.cpp
  src/app/SendMessage.cpp
  src/app/ListMessages.cpp
  src/app/ListConversations.cpp)
target_include_directories(communication_app PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
target_link_libraries(communication_app PUBLIC communication_domain PRIVATE communication_warnings)

if(COMMUNICATION_BUILD_ADAPTERS)
  add_subdirectory(src/adapters)
endif()

if(COMMUNICATION_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
