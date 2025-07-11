#pragma once

#include "globals.hpp"

#include "Peers.hpp"
#include "DB.hpp"
#include "Config.hpp"
#include "Mods.hpp"
#include "Pages.hpp"
#include "LocalUsers.hpp"
#include "Server/Server.hpp"

/**
 * Global protocol server app singleton
 */
class App {

public:
    boost::asio::io_context* m_ioc;

    AppConfig m_config;
    Peers m_peers;
    std::unique_ptr<DB> m_db;
    Mods m_mods;
    std::unique_ptr<Pages> m_pages;
    LocalUsers m_users;
    Server m_server;


    App() = default;
    explicit App(const std::string& config_path):
        m_config(config_path)
    {}

    bool start();
};

// Global Singleton set in main.c
extern App* g_app;