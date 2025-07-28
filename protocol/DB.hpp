#pragma once

#include <mutex>

#include "../third_party/SQLiteCpp/include/SQLiteCpp/SQLiteCpp.h"

#include "Config.hpp"
#include "Peer.hpp"
#include "LocalUser.hpp"

class DB {
public:
    SQLite::Database m_db;

private:
    // Precompiled queries
    SQLite::Statement m_get_user_query;
    SQLite::Statement m_add_user_query;
    SQLite::Statement m_get_peer_query;
    SQLite::Statement m_add_peer_query;

protected:
    std::mutex m_mtx;

public:

    DB();

    std::shared_ptr<Peer> get_peer(std::string domain);

    std::shared_ptr<LocalUser> get_user(const std::string& username);
    std::shared_ptr<LocalUser> get_user(const std::string& username, std::string password);
    bool add_user(const LocalUser& user, std::string password);

};


// TODO db cron that prunes users+servers with expired authentication
