//
// Created by tate on 7/30/25.
//

#pragma once

#include <mutex>

#include "../../../third_party/SQLiteCpp/include/SQLiteCpp/SQLiteCpp.h"


class DB {
protected:
    std::mutex m_mtx;
public:

    SQLite::Database m_db;

    DB();
};
