cmake_minimum_required(VERSION 3.12)

project(fediy
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SQLiteCpp)

find_package(Boost REQUIRED)

file(GLOB protocol_server_sources protocol/*.cpp)

add_executable(protocol_server ${protocol_server_sources}
    third_party/inih/ini.c
    protocol/Mod.cpp
    protocol/Mod.hpp
    protocol/Mods.cpp
    protocol/Mods.hpp
    protocol/LocalUser.cpp
    protocol/Pages.cpp
    protocol/Pages.hpp
    modlib/fediymod.h
    protocol/ModIPC.cpp
    protocol/ModIPC.hpp
    protocol/Server/Server.cpp
    protocol/Server/Server.hpp
    protocol/Server/Router.hpp
    protocol/Server/Router.cpp
    protocol/LocalUsers.hpp
    protocol/LocalUsers.cpp
    util/WebUtils.hpp
    util/WebUtils.cpp
    protocol/HttpsClient.cpp
    protocol/HttpsClient.hpp
    protocol/Server/Session.cpp
    protocol/Server/Session.hpp
    protocol/HttpClient.cpp
    protocol/HttpClient.hpp
    protocol/Server/util.hpp
    util/FileCache.hpp
    util/Crypto.cpp
    util/Crypto.hpp
    protocol/Peer.cpp
)

# Debug/vs release
target_compile_definitions(protocol_server PRIVATE
    $<$<CONFIG:Debug>:FEDIY_DEBUG=1>
)
target_compile_options(protocol_server PRIVATE
    $<$<CONFIG:Debug>:-O0;-g>
    $<$<CONFIG:Release>:-O3>
)

target_include_directories(protocol_server PUBLIC protocol
    third_party/SQLiteCpp/include
    third_party/json/single_include
    third_party/inja/single_include
    third_party/inih
    third_party/Mustache
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(protocol_server
    SQLiteCpp
    sqlite3
    pthread
    dl
    crypto
    ssl
)


add_library(demo_mod SHARED
    modlib/example_mod.c
    modlib/fediymod.h
)

add_library(demo_mod_cpp SHARED
    modlib/example_mod.cpp
    modlib/fediymodpp.hpp
)

add_library(demo_mod_mail SHARED
    apps/mail_mod/mod.cpp
    apps/mail_mod/MailBox.hpp
    modlib/fediymod.h
    modlib/fediymodpp.hpp
)

# Contacts builtin mod
add_library(contacts_mod SHARED
    modlib/fediymod.h
    modlib/fediymodpp.hpp
    apps/contacts/src/mod.cpp
    apps/contacts/src/Contact.cpp
    apps/contacts/src/Contact.hpp
    apps/contacts/src/DB.cpp
    apps/contacts/src/DB.hpp
    apps/contacts/src/Pages.hpp
)
target_link_libraries(contacts_mod
    SQLiteCpp
    sqlite3
    pthread
    dl
)
target_include_directories(contacts_mod PUBLIC protocol
    third_party/SQLiteCpp/include
)

add_library(oauth_mod SHARED
    modlib/fediymod.h
    modlib/fediymodpp.hpp
    apps/oauth/mod.cpp
)

# Code formatting with clang-format
find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
    # Get all C++ source files
    file(GLOB_RECURSE ALL_SOURCE_FILES
        "*.cpp"
        "*.hpp"
        "*.h"
        "*.c"
    )
    
    # Exclude third_party directory
    list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX "third_party/.*")
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on all source files"
    )
    
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking clang-format on all source files"
    )
else()
    message(WARNING "clang-format not found. Install clang-format to enable code formatting.")
endif()
