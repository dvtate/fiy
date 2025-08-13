//
// Created by tate on 7/30/25.
//

#pragma once

#include <vector>

#include "../../../third_party/SQLiteCpp/include/SQLiteCpp/SQLiteCpp.h"

namespace DB {
    using Exception = SQLite::Exception;

    SQLite::Database& connection();

    std::string get_profile(
        const std::string& user,
        const std::string& req_user,
        const std::string& req_domain
    );
} // namespace DB
