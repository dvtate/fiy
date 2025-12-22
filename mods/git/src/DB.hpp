//
// Created by tate on 12/21/25.
//

#pragma once

#include <string>

#include "../../../third_party/SQLiteCpp/include/SQLiteCpp/SQLiteCpp.h"

#include "../../../modlib/fediymod.hpp"

namespace DB {

    enum class Result {
        Success,
        Unauthorized,
        DbError,
        NetError
    };

    inline SQLite::Database& connection() {
        thread_local SQLite::Database db{
            std::string(fiy::Host::info.data_dir) + "/db.db3",
            SQLite::OPEN_READWRITE
        };
        return db;
    }
    inline SQLite::Statement statement(const char* sql) {
        return SQLite::Statement{ connection(), sql };
    }
    inline SQLite::Statement operator ""_sql(const char* str, std::size_t) {
        return statement(str);
    }

    inline void transaction_begin() {
        thread_local auto q = "BEGIN TRANSACTION"_sql;
        q.exec();
        q.reset();
    }

    inline void transaction_commit() {
        thread_local auto q = "COMMIT"_sql;
        q.exec();
        q.reset();
    }

    inline void transaction_rollback() {
        thread_local auto q = "ROLLBACK"_sql;
        q.exec();
        q.reset();
    }

}