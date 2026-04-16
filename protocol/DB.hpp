#pragma once

#include "../third_party/SQLiteCpp/include/SQLiteCpp/SQLiteCpp.h"

namespace DB {
    using Exception = SQLite::Exception;

    enum class Result {
        Success,
        Unauthorized,
        DbError,
        NetError
    };

    /**
     * Get a connection to the database
     * 
     * @note creates only one connection per thread, then re-uses
     */
    SQLite::Database& connection();

    inline SQLite::Statement operator ""_sql(const char* str, std::size_t) {
        return SQLite::Statement{ connection(), str };
    }

    /**
     * Start a transaction
     */
    inline void transaction_begin() {
        thread_local auto q = "BEGIN TRANSACTION"_sql;
        q.exec();
        q.reset();
    }

    /**
     * Complete a transaction
     */
    inline void transaction_commit() {
        thread_local auto q = "COMMIT"_sql;
        q.exec();
        q.reset();
    }

    /**
     * Cancel a transaction
     */
    inline void transaction_rollback() {
        thread_local auto q = "ROLLBACK"_sql;
        q.exec();
        q.reset();
    }

}

// TODO db cron that prunes expired authentication tokens
