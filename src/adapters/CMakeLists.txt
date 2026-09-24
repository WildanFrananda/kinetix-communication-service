# The adapters, and the executable that wires them together.
#
# Everything in this file is a dependency on somebody else's library. That is the whole reason it
# is a separate CMakeLists guarded by COMMUNICATION_BUILD_ADAPTERS: the layers that hold this service's
# rules build and test with nothing but a compiler, and this is the part that needs Qt, gRPC,
# Drogon and Boost present before it can say anything at all.

find_package(Qt6 REQUIRED COMPONENTS Core Sql)
find_package(Drogon REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

# gRPC's own config file names c-ares and OpenSSL in its link interface but does not look for
# them, so importing gRPC fails with "target was not found" unless they are imported first.
find_package(c-ares CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Boost REQUIRED)

include(FetchContent)

# --------------------------------------------------------------------------------------------
# TinyORM, built as part of this build rather than found as an installed package.
#
# `find_package(TinyOrm)` does not work against an install tree here, and the reason is worth
# writing down so nobody spends the afternoon on it again: TinyOrmConfigVersion.cmake asks a
# helper for the installed target configurations, and that helper locates them relative to
# CMAKE_PARENT_LIST_FILE — which, during find_package, is the *consuming* project's
# CMakeLists.txt. The glob finds nothing, the package is reported as "no-configuration", and
# CMake refuses it while the files sit right there. Measured on TinyORM 0.38.1.
#
# Building it here also removes a manual install step from every machine that builds this
# service, and pins the version in the same place as every other dependency.
#
# Pinned to a commit as well as a tag: a tag can be moved, and a moved tag would silently change
# what this service links against.
# --------------------------------------------------------------------------------------------
set(TINYORM_TAG v0.38.1)
set(TINYORM_COMMIT 2827c6bd24d2eb622e39a8654bace4a7e35921c0)

set(TOM OFF CACHE BOOL "" FORCE)            # the tom CLI is a scaffolding tool, not a runtime need
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)    # TinyORM's own suite is TinyORM's business
set(BUILD_DRIVERS OFF CACHE BOOL "" FORCE)  # TinyDrivers only finishes MySQL; Postgres goes via QtSql

FetchContent_Declare(TinyOrm
  GIT_REPOSITORY https://github.com/silverqx/TinyORM.git
  GIT_TAG ${TINYORM_COMMIT}
  GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(TinyOrm)

# --------------------------------------------------------------------------------------------
# The contract, generated at build time.
#
# There is no C++ package published from kinetix-contracts — buf's remote plugins do not emit C++
# — so this service generates the stubs from the .proto that bin/sync-contracts fetched at a
# pinned tag and commit. `.contracts/` is gitignored, so no .proto is committed here, which is
# what keeps the S9 rule (the contract repository is the only source) true for this repository
# too.
# --------------------------------------------------------------------------------------------
# Overridable so that a compile check can be run against a patched copy of the contract
# without editing the fetched one. It is never overridden in a real build.
set(CONTRACTS_DIR ${CMAKE_SOURCE_DIR}/.contracts/spoken CACHE PATH
    "Where the synced .proto files live")
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/contracts)

if(NOT EXISTS ${CONTRACTS_DIR}/identity/v1/identity.proto)
  message(FATAL_ERROR
    "${CONTRACTS_DIR} is empty. Run bin/sync-contracts before configuring — it fetches the "
    "contract at the tag and commit this service is pinned to. Do not hand-write the .proto.")
endif()

set(PROTO_FILES
  ${CONTRACTS_DIR}/common/v1/common.proto
  ${CONTRACTS_DIR}/identity/v1/identity.proto)

set(GENERATED_SOURCES
  ${GENERATED_DIR}/common/v1/common.pb.cc
  ${GENERATED_DIR}/identity/v1/identity.pb.cc
  ${GENERATED_DIR}/identity/v1/identity.grpc.pb.cc)

set(GENERATED_HEADERS
  ${GENERATED_DIR}/common/v1/common.pb.h
  ${GENERATED_DIR}/identity/v1/identity.pb.h
  ${GENERATED_DIR}/identity/v1/identity.grpc.pb.h)

file(MAKE_DIRECTORY ${GENERATED_DIR})

add_custom_command(
  OUTPUT ${GENERATED_SOURCES} ${GENERATED_HEADERS}
  COMMAND protobuf::protoc
    --proto_path=${CONTRACTS_DIR}
    --cpp_out=${GENERATED_DIR}
    --grpc_out=${GENERATED_DIR}
    --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
    ${PROTO_FILES}
  DEPENDS ${PROTO_FILES} protobuf::protoc gRPC::grpc_cpp_plugin
  COMMENT "Generating C++ stubs from the pinned contract"
  VERBATIM)

add_library(communication_contracts STATIC ${GENERATED_SOURCES})
target_include_directories(communication_contracts PUBLIC ${GENERATED_DIR})
target_link_libraries(communication_contracts PUBLIC gRPC::grpc++ protobuf::libprotobuf)

# Generated code is not this repository's code, and holding it to this repository's warnings would
# make the rule about something nobody wrote.
target_compile_options(communication_contracts PRIVATE -w)

# --------------------------------------------------------------------------------------------
# mqtt_cpp is header-only and is not packaged by Homebrew, so it is fetched at a pinned tag.
# --------------------------------------------------------------------------------------------
set(MQTT_CPP_TAG v13.2.3)
set(MQTT_CPP_COMMIT 282b0aac345793f785cfc37abced6ec019cebe1c)

# SOURCE_SUBDIR names a directory that does not exist on purpose: mqtt_cpp is header-only, so its
# sources are wanted but its CMakeLists — which builds its examples and its own test suite — is
# not. This is the supported way to download without configuring; FetchContent_Populate does the
# same thing and is deprecated.
FetchContent_Declare(mqtt_cpp
  GIT_REPOSITORY https://github.com/redboltz/mqtt_cpp.git
  GIT_TAG ${MQTT_CPP_COMMIT}
  GIT_SHALLOW FALSE
  SOURCE_SUBDIR headers-only-do-not-configure)
FetchContent_MakeAvailable(mqtt_cpp)

add_library(communication_mqtt_cpp INTERFACE)
target_include_directories(communication_mqtt_cpp SYSTEM INTERFACE ${mqtt_cpp_SOURCE_DIR}/include)
target_link_libraries(communication_mqtt_cpp INTERFACE Boost::boost)
# MQTT_NS is the namespace macro mqtt_cpp expects to be told about; without it the headers pick a
# default that differs between versions.
target_compile_definitions(communication_mqtt_cpp INTERFACE MQTT_STD_VARIANT MQTT_STD_OPTIONAL
                                                   MQTT_STD_STRING_VIEW)

# --------------------------------------------------------------------------------------------
# The adapters themselves.
# --------------------------------------------------------------------------------------------
add_library(communication_adapters STATIC
  grpc/GrpcPrincipalAuthenticator.cpp
  grpc/GrpcPrincipalDirectory.cpp
  http/CommunicationController.cpp
  http/RequestWorkers.cpp
  mqtt/MqttMessageBroadcaster.cpp
  persistence/PostgresConversationRepository.cpp
  persistence/PostgresMessageRepository.cpp
  persistence/Sql.cpp
  system/Settings.cpp
  wire/Json.cpp)

target_link_libraries(communication_adapters
  PUBLIC communication_app communication_contracts communication_mqtt_cpp Drogon::Drogon TinyOrm::TinyOrm Qt6::Core Qt6::Sql
  PRIVATE communication_warnings)

add_executable(kinetix-communication-service ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(kinetix-communication-service PRIVATE communication_adapters communication_warnings)
