#pragma once

#include <ctime>

#include "defs.hpp"

#include "peers/Peers.hpp"
#include "Config.hpp"
#include "mods/Mods.hpp"
#include "users/LocalUsers.hpp"
#include "HttpClient.hpp"
#include "HttpsClient.hpp"

class FIY;

// Global Singleton set in main.cpp
extern FIY* g_fiy;

/**
 * Global protocol server app singleton
 */
class FIY {
    volatile std::time_t m_now{0};

public:
    boost::asio::io_context* m_ioc{nullptr};

    FiyConfig m_config;
    Peers m_peers;
    Mods m_mods;
    LocalUsers m_users;
    HttpClient m_http;
    HttpsClient m_https;

    FIY() = default;
    explicit FIY(const std::string& config_path):
        m_config(config_path)
    {}


    bool start(int argc, char* argv[]);
    bool start();


    std::time_t now() const {
        return m_now;
    }

    [[nodiscard]] const std::string& base_uri() const;
};
