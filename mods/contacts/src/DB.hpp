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
        const char* req_user,
        const char* req_domain
    );

    int64_t get_profile_id(const std::string& user);

    enum Result {
        Success,
        Unauthorized,
        Error
    };
    Result save_contact(VC& card);

    std::string get_user_rolodex(const std::string& local_user);

    std::string get_pfp(
        const std::string& user,
        const char* req_user,
        const char* req_domain
    );

    bool get_contact(VC& card);

} // namespace DB
