# GoogleTest comes from vcpkg when the adapters are being built, and is fetched otherwise.
#
# The fetch exists so that the domain and application tests run on a machine that has never
# bootstrapped vcpkg. That is not a convenience: a test suite that needs a four-hour dependency
# build before it can report anything is a suite that stops being run.
find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  # GoogleTest declares the GTest:: aliases itself; adding them here collides with its own.
  FetchContent_MakeAvailable(googletest)
endif()

add_library(communication_test_support INTERFACE)
target_include_directories(communication_test_support INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(communication_test_support INTERFACE communication_domain GTest::gmock)

add_executable(communication_unit_tests
  unit/TextTest.cpp
  unit/ValueTypeTest.cpp
  unit/MessageBodyTest.cpp
  unit/ConversationTest.cpp
  unit/OpenConversationTest.cpp
  unit/SendMessageTest.cpp
  unit/ListMessagesTest.cpp
  unit/ListConversationsTest.cpp
  # An adapter, but one that needs nothing outside the standard library, so it is checked here
  # rather than left to the build that takes hours.
  unit/UuidIdentifierSourceTest.cpp)
target_link_libraries(communication_unit_tests PRIVATE
  communication_app communication_test_support GTest::gtest GTest::gmock GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(communication_unit_tests)
