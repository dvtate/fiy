//
// Created by tate on 7/30/25.
//

#pragma once

#include <mutex>
#include <vector>

#include "../../../third_party/SQLiteCpp/include/SQLiteCpp/SQLiteCpp.h"

#include "Contact.hpp"

class DB {
protected:
    std::mutex m_mtx;

    SQLite::Statement m_get_user_contacts;

public:
    using Exception = SQLite::Exception;

    SQLite::Database m_db;

    DB();

    std::vector<Contact> get_contacts(const std::string_view& owner);

    bool get_user_profile(const std::string_view& user, int level, Contact& c);

    Contact get_contact_by_id(int64_t id);
};

extern DB* g_db;