//
// Created by tate on 7/30/25.
//

#pragma once

#include <vector>

#include "../../../third_party/SQLiteCpp/include/SQLiteCpp/SQLiteCpp.h"

#include "Contact.hpp"

namespace DB {
    using Exception = SQLite::Exception;

    SQLite::Database& connection();
    void transaction_begin();
    void transaction_commit();


    std::string get_profile(
        const std::string& user,
        const std::string& req_user,
        const std::string& req_domain
    );

    int64_t get_profile_id(const std::string& user);

    bool save_contact(VC& card);
} // namespace DB
