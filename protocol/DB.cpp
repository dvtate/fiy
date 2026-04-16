#include <openssl/sha.h>

#include "DB.hpp"

#include "FIY.hpp"

namespace DB {

    SQLite::Database& connection() {
        // By the time the db gets used the config should already be initialized
        thread_local SQLite::Database db{
            g_fiy->config.data_dir + "/db.db3",
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        };
        return db;
    }

    // std::shared_ptr<Peer> get_peer(const std::string_view domain) {
    //     thread_local auto query = "SELECT symKey, giveToken, takeToken, tokenExpireTs"
    //         " FROM Peers WHERE domain = ?"_sql;
    //     CleanupRoutine cleanup{[&](){
    //         query.clearBindings();
    //         query.reset();
    //     }};
    //
    //     query.bind(1, std::string(domain));
    //     if (!query.executeStep()) // no match
    //         return nullptr;
    //
    //     return std::make_shared<Peer>(
    //         std::string(domain),
    //         PeerAuth{
    //             query.getColumn(0).getString(),
    //             query.getColumn(0).getString(),
    //             query.getColumn(0).getString(),
    //             (std::time_t)query.getColumn(0).getInt64()
    //         }
    //     );
    // }

    // TODO add_peer
    // INSERT INTO Peers (domain, connectTs, bearerToken, symKey, pubkey, tokenExpireTs) VALUES (?, ?, ?, ?, ?, ?)

} // namespace DB