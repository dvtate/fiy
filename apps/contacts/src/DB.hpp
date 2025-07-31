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

    SQLite::Database m_db;

    DB();

    std::vector<Contact> get_contacts(const std::string& owner);
};

extern DB* g_db;