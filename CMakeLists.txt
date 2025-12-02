cmake_minimum_required(VERSION 3.12)

project(fediy
        LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Og")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SQLiteCpp)

find_package(Boost REQUIRED COMPONENTS url)

file(GLOB protocol_server_sources protocol/*.cpp)

add_executable(protocol_server ${protocol_server_sources}
    third_party/inih/ini.c
    modlib/fediymod.h
    protocol/defs.hpp
    protocol/FIY.hpp
    protocol/FIY.cpp
    protocol/Mod.cpp
    protocol/Mod.hpp
    protocol/Mods.cpp
    protocol/Mods.hpp
    protocol/ModConnector.cpp
    protocol/ModConnector.hpp
    protocol/LocalUser.cpp
    protocol/Pages.cpp
    protocol/Pages.hpp
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
    gpgme
    Boost::url
)

add_library(fediymod SHARED
    modlib/fediymod.h
    modlib/fediymod.hpp
    modlib/fediymod.cpp
)


add_library(demo_mod SHARED
    modlib/example_mod.c
    modlib/fediymod.h
)
target_link_libraries(demo_mod PRIVATE fediymod)

add_library(demo_mod_cpp SHARED
    modlib/fediymod.h
    modlib/fediymod.hpp
    modlib/example_mod.cpp
)
target_link_libraries(demo_mod_cpp PRIVATE fediymod)

add_library(demo_mod_mail SHARED
    modlib/fediymod.h
    modlib/fediymod.hpp
    mods/mail/MailBox.hpp
    mods/mail/mod.cpp
)
target_link_libraries(demo_mod_mail PRIVATE fediymod)

# Contacts builtin mod
add_library(contacts_mod SHARED
    modlib/fediymod.h
    modlib/fediymod.hpp
    mods/contacts/src/mod.cpp
    mods/contacts/src/Contact.cpp
    mods/contacts/src/Contact.hpp
    mods/contacts/src/DB.cpp
    mods/contacts/src/DB.hpp
    mods/contacts/src/Pages.hpp
    third_party/base64.hpp
    mods/contacts/src/timezones.hpp
)
target_link_libraries(contacts_mod
    PRIVATE fediymod
    PUBLIC SQLiteCpp
    PUBLIC sqlite3
    PUBLIC pthread
    PUBLIC dl
)


target_include_directories(contacts_mod PUBLIC
    third_party/SQLiteCpp/include
)

add_library(oauth_mod SHARED
    modlib/fediymod.h
    modlib/fediymod.hpp
    mods/oauth/mod.cpp
)
target_link_libraries(demo_mod_mail PRIVATE fediymod)
