#pragma once

#include "../third_party/SQLiteCpp/include/SQLiteCpp/SQLiteCpp.h"

#include "Config.hpp"
#include "LocalUser.hpp"
#include "Peer.hpp"

namespace DB {
using Exception = SQLite::Exception;

/**
 * Get a connection to the database
 *
 * @note creates only one connection per thread, then re-uses
 */
SQLite::Database& connection();

std::shared_ptr<LocalUser> get_user(const std::string& username);
std::shared_ptr<LocalUser> get_user(const std::string& username, std::string password);
bool add_user(const LocalUser& user, std::string password);
std::shared_ptr<Peer> get_peer(const std::string_view domain);

};  // namespace DB

// TODO db cron that prunes users+servers with expired authentication
