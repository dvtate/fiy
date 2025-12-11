#pragma once

#include <ctime>

#include "defs.hpp"

#include "Peers.hpp"
#include "DB.hpp"
#include "Config.hpp"
#include "Mods.hpp"
#include "Pages.hpp"
#include "LocalUsers.hpp"
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

    FediyConfig m_config;
    Peers m_peers;
    Mods m_mods;
    std::unique_ptr<Pages> m_pages;
    LocalUsers m_users;
    HttpClient m_http;
    HttpsClient m_https;

    FIY() = default;
    explicit FIY(const std::string& config_path):
        m_config(config_path)
    {}

    bool start();

    inline std::time_t now() const {
        return m_now;
    }

    [[nodiscard]] const std::string& base_uri() const;
};
