cmake_minimum_required(VERSION 3.12)

project(fediy
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SQLiteCpp)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/drogon)

file(GLOB protocol_server_sources protocol/*.cpp)

add_executable(protocol_server ${protocol_server_sources}
    third_party/inih/ini.c
    protocol/ProtocolActions.hpp
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
    protocol/Server/Connection.hpp
    protocol/Server/Connection.cpp
    util/Cookie.hpp
    util/Cookie.cpp
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
    third_party/drogon/lib/inc
)

target_link_libraries(protocol_server
    SQLiteCpp
    sqlite3
    pthread
    dl
    crypto
    drogon
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
    mail_mod/mod.cpp
    mail_mod/MailBox.hpp
    modlib/fediymod.h
    modlib/fediymodpp.hpp
)