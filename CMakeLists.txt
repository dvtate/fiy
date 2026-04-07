cmake_minimum_required(VERSION 3.12)

project(fiy
        LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# C++ Flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Og -DFIY_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_STANDARD 20)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    file(READ "/etc/os-release" OS_RELEASE_CONTENT)
    # Skip static libs on Fedora
    if(NOT OS_RELEASE_CONTENT MATCHES "ID=.*fedora")
        set(Boost_USE_STATIC_LIBS ON)
    endif()
else()
    set(Boost_USE_STATIC_LIBS ON)
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/SQLiteCpp)

find_package(Boost REQUIRED COMPONENTS url context)

file(GLOB protocol_server_sources protocol/*.cpp)

add_executable(protocol_server ${protocol_server_sources}
    third_party/inih/ini.c
    modlib/fiymod.h
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
    $<$<CONFIG:Debug>:FIY_DEBUG=1>
)
#target_compile_options(protocol_server PRIVATE
#    $<$<CONFIG:Debug>:-O0;-g>
#    $<$<CONFIG:Release>:-O3>
#)

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
    Boost::url
)

add_library(fiymod SHARED
    modlib/fiymod.h
    modlib/fiymod.hpp
    modlib/fiymod.cpp
)


add_library(demo_mod SHARED
    modlib/example_mod.c
    modlib/fiymod.h
)
target_link_libraries(demo_mod PRIVATE fiymod)
target_compile_options(demo_mod PRIVATE
    -fvisibility=hidden
)

add_library(demo_mod_cpp SHARED
    modlib/fiymod.h
    modlib/fiymod.hpp
    modlib/example_mod.cpp
)
target_link_libraries(demo_mod_cpp PRIVATE fiymod)
target_compile_options(demo_mod_cpp PRIVATE
    -fvisibility=hidden
)

add_library(demo_mod_mail SHARED
    modlib/fiymod.h
    modlib/fiymod.hpp
    mods/mail/MailBox.hpp
    mods/mail/mod.cpp
)
target_link_libraries(demo_mod_mail PRIVATE fiymod)
target_compile_options(demo_mod_mail PRIVATE
    -fvisibility=hidden
)

# Builtin contacts mod
add_library(contacts_mod SHARED
    modlib/fiymod.h
    modlib/fiymod.hpp
    mods/contacts/src/mod.cpp
    mods/contacts/src/Contact.cpp
    mods/contacts/src/Contact.hpp
    mods/contacts/src/DB.cpp
    mods/contacts/src/DB.hpp
    mods/contacts/src/Pages.hpp
    mods/contacts/src/timezones.hpp
    third_party/base64.hpp
)
target_link_libraries(contacts_mod
    PRIVATE fiymod
    PUBLIC SQLiteCpp
    PUBLIC sqlite3
    PUBLIC pthread
    PUBLIC dl
)
target_include_directories(contacts_mod PUBLIC
    third_party/SQLiteCpp/include
)
target_compile_options(contacts_mod PRIVATE
    -fvisibility=hidden
)

# Builtin git builtin mod
add_library(git_mod SHARED
    modlib/fiymod.h
    modlib/fiymod.hpp
    mods/git/src/mod.cpp
    mods/git/src/Routes/Pages.hpp
    mods/git/src/Repos/git_http_backend.cpp
    mods/git/src/Repos/git_http_backend.hpp
    mods/git/src/Repos/LocalRepo.cpp
    mods/git/src/Repos/LocalRepo.hpp
    mods/git/src/Repos/Repos.cpp
    mods/git/src/Repos/Repos.hpp
    mods/git/src/DB.cpp
    mods/git/src/DB.hpp
    util/WebUtils.cpp
    util/WebUtils.hpp
    mods/git/src/Repos/GitRepo.cpp
    mods/git/src/Repos/GitRepo.hpp
    mods/git/src/Repos/BasicRepo.cpp
    mods/git/src/Repos/BasicRepo.hpp
    mods/git/src/Routes/RepoPageData.cpp
    mods/git/src/Routes/RepoPageData.hpp
    mods/git/src/Routes/RepoRouter.cpp
    mods/git/src/Routes/RepoRouter.hpp
    mods/git/src/Routes/Router.hpp
    mods/git/src/Repos/RemoteRepo.cpp
    mods/git/src/Repos/RemoteRepo.hpp
    mods/git/src/Routes/ApiRouter.cpp
    mods/git/src/Routes/ApiRouter.hpp
    mods/git/src/Routes/AssetRouter.cpp
    mods/git/src/Routes/AssetRouter.hpp
    mods/git/src/Routes/UserPageData.cpp
    mods/git/src/Routes/UserPageData.hpp
    mods/git/src/Repos/RepoSearch.hpp
    mods/git/src/Repos/RepoSearch.cpp
)
target_link_libraries(git_mod
    PRIVATE fiymod
    PUBLIC SQLiteCpp
    PUBLIC sqlite3
    PUBLIC pthread
    PUBLIC git2
    PUBLIC dl
)
target_compile_options(git_mod PRIVATE
    -fvisibility=hidden
)

# Builtin http static server mod
add_library(static_mod SHARED
    modlib/fiymod.h
    modlib/fiymod.hpp
    util/mime.hpp
    util/mime.cpp
    mods/static/mod.cpp
)
target_link_libraries(static_mod PRIVATE fiymod)
target_compile_options(static_mod PRIVATE
    -fvisibility=hidden
)

# Builtin http reverse proxy mod
add_library(reverse_proxy_mod SHARED
    modlib/fiymod.h
    modlib/fiymod.hpp
    mods/reverse_proxy/mod.cpp
)
target_link_libraries(reverse_proxy_mod PRIVATE fiymod)
target_compile_options(reverse_proxy_mod PRIVATE
    -fvisibility=hidden
)

# Builtin oauth mod
add_library(oauth_mod SHARED
    modlib/fiymod.h
    modlib/fiymod.hpp
    mods/oauth/mod.cpp
)
target_link_libraries(oauth_mod PRIVATE fiymod)
target_compile_options(oauth_mod PRIVATE
    -fvisibility=hidden
)

# Builtin notifications mod
add_library(notifications_mod SHARED
    modlib/fiymod.h
    modlib/fiymod.hpp
    mods/notifications/mod.cpp
)
target_link_libraries(notifications_mod PRIVATE fiymod)
target_compile_options(notifications_mod PRIVATE
    -fvisibility=hidden
)

add_executable(fiy_cdn mods/storage/cdn/main.cpp)
target_compile_definitions(fiy_cdn PRIVATE
    $<$<CONFIG:Debug>:FIY_DEBUG=1>
)
#target_compile_options(fiy_cdn PRIVATE
#    $<$<CONFIG:Debug>:-O0;-g>
#    $<$<CONFIG:Release>:-O3>
#)

target_include_directories(fiy_cdn PUBLIC
    third_party/SQLiteCpp/include
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(fiy_cdn
    SQLiteCpp
    sqlite3
    pthread
    crypto
    ssl
    curl
    Boost::headers
    Boost::url
    Boost::context
)

add_library(social_mod SHARED
    modlib/fiymod.h
    modlib/fiymod.hpp
    mods/social/src/mod.cpp
    mods/social/src/Post.hpp
    mods/social/src/Post.cpp
    mods/social/src/Posts.hpp
    mods/social/src/Posts.hpp
)
target_link_libraries(social_mod PRIVATE fiymod)
target_compile_options(social_mod PRIVATE
    -fvisibility=hidden
)