//
// Created by tate on 12/21/25.
//

#pragma once

#include <string>

#include "../../../third_party/SQLiteCpp/include/SQLiteCpp/SQLiteCpp.h"

#include "../../../modlib/fiymod.hpp"

namespace DB {

    enum class Result {
        Success,
        Unauthorized,
        DbError,
        NetError
    };

    inline SQLite::Database& connection() {
        thread_local SQLite::Database db{
            std::string(fiy::host().data_dir) + "/db.db3",
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

    /// Used to make statements that with too many possibilities to be precompiled
    struct QueryBuilder {
        using BindFunction = std::function<void(SQLite::Statement&, int&)>;
        std::string m_stmt;

        std::vector<BindFunction> m_bind_params;

        // Add a part and a function that gets called when binding params
        void part(const std::string_view part, BindFunction func) {
            m_stmt += part;
            m_bind_params.emplace_back(std::move(func));
        }
        void part(const std::string_view part) {
            m_stmt += part;
        }

        // Used to build where clause
        bool m_first_cond = true;
        void and_cond() {
            if (m_first_cond) {
                part(" WHERE ");
                m_first_cond = false;
            } else {
                part(" AND ");
            }
        }
        void or_cond() {
            if (m_first_cond) {
                part(" WHERE ");
                m_first_cond = false;
            } else {
                part(" OR ");
            }
        }

        SQLite::Statement build(SQLite::Database& db = DB::connection()) {
            SQLite::Statement ret(db, m_stmt);
            int index = 1;
            for (auto& f : m_bind_params)
                f(ret, index);
            return ret;
        }
    };

}