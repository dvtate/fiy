#include <openssl/sha.h>

#include "DB.hpp"

#include "App.hpp"

namespace DB {

template <class T>
struct CleanupRoutine {
    T m_action;
    explicit CleanupRoutine(T f) : m_action(std::move(f)) {}
    ~CleanupRoutine() {
        m_action();
    }
};

SQLite::Database& connection() {
    // By the time the db gets used the config should already be initialized
    static thread_local SQLite::Database db{g_app->m_config.m_data_dir + "/db.db3",
                                            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
    return db;
}

static SQLite::Statement& get_user_by_username_query() {
    static thread_local SQLite::Statement query{
        connection(),
        "SELECT username, isAdmin, name, hashedPassword, email, locale, joinTs "
        "FROM Users WHERE username = ?"};
    return query;
}

std::shared_ptr<LocalUser> get_user(const std::string& username, std::string password) {
    // Get query for this thread
    auto& query = get_user_by_username_query();
    CleanupRoutine cleanup{[&]() {
        query.clearBindings();
        query.reset();
    }};

    // Fetch relevant user from database
    query.bindNoCopy(1, username);
    if (!query.executeStep())  // no match
        return nullptr;

    // Get password hash from database
    const auto pw_col = query.getColumn(3);
    if (pw_col.getBytes() < SHA512_DIGEST_LENGTH) {
        LOG_ERR("DB: hashedPassword column shorter than expected???: " << pw_col.getBytes());
        return nullptr;
    }

    // Hash provided password
    // TODO switch to argon2 or something better
    unsigned char hashed_password[SHA512_DIGEST_LENGTH];
    password += g_app->m_config.m_salt;
    unsigned char* hp = SHA512((unsigned char*)password.data(), password.size(), hashed_password);

    // Verify that password is correct
    auto db_hp = (unsigned char*)pw_col.getBlob();
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
        if (hp[i] != db_hp[i]) {
            DEBUG_LOG(i << ":  '" << hp[i] << "' != '" << db_hp[i] << "'");
            return nullptr;  // wrong pw
        }

    // Make user object
    return std::make_shared<LocalUser>(username,
                                       query.getColumn(1).getInt() != 0,  // isAdmin
                                       query.getColumn(2).getString(),    // name
                                       query.getColumn(4).getString(),    // email
                                       query.getColumn(5).getString(),    // locale
                                       query.getColumn(6).getUInt()       // join_ts
    );
}

std::shared_ptr<LocalUser> get_user(const std::string& username) {
    // Get query for this thread
    auto& query = get_user_by_username_query();
    CleanupRoutine cleanup{[&]() {
        query.clearBindings();
        query.reset();
    }};

    // Fetch user from db
    query.bindNoCopy(1, username);
    if (!query.executeStep())  // no match
        return nullptr;
    return std::make_shared<LocalUser>(username,
                                       query.getColumn(1).getInt() != 0,  // isAdmin
                                       query.getColumn(2).getString(),    // name
                                       query.getColumn(4).getString(),    // email
                                       query.getColumn(5).getString(),    // locale
                                       query.getColumn(6).getUInt()       // join_ts
    );
}

bool add_user(const LocalUser& user, std::string password) {
    static thread_local SQLite::Statement q{
        connection(),
        "INSERT INTO Users (username, isAdmin, name, hashedPassword, email, locale, joinTs) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"};

    // Hash provided password
    unsigned char hashed_password[SHA512_DIGEST_LENGTH];
    password += g_app->m_config.m_salt;
    unsigned char* hp = SHA512((unsigned char*)password.c_str(), password.size(), hashed_password);
    // TODO could this error?

    q.bindNoCopy(1, user.get_username());
    q.bind(2, (int32_t)user.m_is_admin);
    q.bindNoCopy(3, user.get_name());
    q.bind(4, (void*)hp, 128);
    q.bindNoCopy(5, user.m_email);
    q.bindNoCopy(6, user.m_locale);
    q.bind(7, (int64_t)user.m_joined_ts);
    auto ret = q.exec();
    q.clearBindings();
    q.reset();
    return ret;
}

std::shared_ptr<Peer> get_peer(const std::string_view domain) {
    static thread_local SQLite::Statement query{
        connection(),
        "SELECT symKey, giveToken, takeToken, tokenExpireTs FROM Peers WHERE domain = ?"};
    CleanupRoutine cleanup{[&]() {
        query.clearBindings();
        query.reset();
    }};

    query.bind(1, std::string(domain));
    if (!query.executeStep())  // no match
        return nullptr;

    return std::make_shared<Peer>(
        std::string(domain),
        PeerAuth{query.getColumn(0).getString(), query.getColumn(0).getString(),
                 query.getColumn(0).getString(), (std::time_t)query.getColumn(0).getInt64()});
}

// TODO add_peer
// INSERT INTO Peers (domain, connectTs, bearerToken, symKey, pubkey, tokenExpireTs) VALUES (?, ?,
// ?, ?, ?, ?)

}  // namespace DB