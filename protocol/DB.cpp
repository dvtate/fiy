#include <openssl/sha.h>

#include "DB.hpp"

#include "App.hpp"

DB::DB():
    m_db(g_app->m_config.m_data_dir + "/db.db3", SQLite::OPEN_FULLMUTEX | SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE ),
    m_get_user_query(m_db, "SELECT username, isAdmin, name, hashedPassword, email, locale, joinTs, about FROM Users WHERE username = ?"),
    m_add_user_query(m_db, "INSERT INTO Users (username, isAdmin, name, hashedPassword, email, locale, joinTs, about) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"),
    m_get_peer_query(m_db, "SELECT * FROM Peers WHERE domain = ?"),
    m_add_peer_query(m_db, "INSERT INTO Peers (domain, connectTs, bearerToken, symKey, pubkey, tokenExpireTs) VALUES (?, ?, ?, ?, ?, ?)")
{
}

template<class T>
struct CleanupRoutine {
    T m_action;
    explicit CleanupRoutine(T f): m_action(std::move(f)) {}
    ~CleanupRoutine() { m_action(); }
};

class DBQueryThreadSafety {
    SQLite::Statement& m_stmt;
    std::mutex& m_mtx;
public:
    DBQueryThreadSafety(
        SQLite::Statement& statement,
        std::mutex& mutex
    ): m_stmt(statement), m_mtx(mutex) {
        mutex.lock();
    }
    ~DBQueryThreadSafety() {
        m_stmt.clearBindings();
        m_stmt.reset();
        m_mtx.unlock();
    }
};

std::shared_ptr<LocalUser> DB::get_user(const std::string& username, std::string password) {
    // TODO should hash password while waiting for DB response

    // Hash provided password
    unsigned char hashed_password[SHA512_DIGEST_LENGTH];
    password += g_app->m_config.m_salt;

    // TODO eventually switch to argon2 or something better
    unsigned char* hp = SHA512((unsigned char*) password.c_str(), password.size(), hashed_password);

    // Thread safety
    DBQueryThreadSafety lock{m_get_user_query, m_mtx};

    // Fetch relevant user from database
    m_get_user_query.bindNoCopy(1, username);
    if (!m_get_user_query.executeStep()) // no match
        return nullptr;

    // Verify that password is correct
    const auto pw_col = m_get_user_query.getColumn(3);
    if (pw_col.getBytes() < SHA512_DIGEST_LENGTH) {
        LOG_ERR("DB: hashedPassword column shorter than expected???: " <<pw_col.getBytes());
        return nullptr;
    }
    auto db_hp = (unsigned char*) pw_col.getBlob();
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
        if (hp[i] != db_hp[i]) {
            DEBUG_LOG(i << ":  '" << hp[i] << "' != '" << db_hp[i] << "'");
            return nullptr; // wrong pw
        }

    // Make user object
    return std::make_shared<LocalUser>(
        username,
        m_get_user_query.getColumn(1).getInt() != 0, // isAdmin
        m_get_user_query.getColumn(2).getString(),  // name
        m_get_user_query.getColumn(4).getString(),  // email
        m_get_user_query.getColumn(5).getString(),  // locale
        m_get_user_query.getColumn(6).getUInt()  // join_ts
    );
}

std::shared_ptr<LocalUser> DB::get_user(const std::string& username) {
    // Thread safety
    DBQueryThreadSafety lock{m_get_user_query, m_mtx};

    // Fetch user from db
    m_get_user_query.bindNoCopy(1, username);
    if (!m_get_user_query.executeStep()) // no match
        return nullptr;
    return std::make_shared<LocalUser>(
        username,
        m_get_user_query.getColumn(1).getInt() != 0, // isAdmin
        m_get_user_query.getColumn(2).getString(),  // name
        m_get_user_query.getColumn(4).getString(),  // email
        m_get_user_query.getColumn(5).getString(),  // locale
        m_get_user_query.getColumn(6).getUInt()  // join_ts
    );
}

bool DB::add_user(const LocalUser& user, std::string password) {
    // Hash provided password
    unsigned char hashed_password[SHA512_DIGEST_LENGTH];
    password += g_app->m_config.m_salt;
    unsigned char* hp = SHA512((unsigned char*) password.c_str(), password.size(), hashed_password);

    // Thread safety
    auto& q = m_add_user_query;
    DBQueryThreadSafety lock{q, m_mtx};

    q.bindNoCopy(1, user.get_username());
    q.bind(2, (int32_t)user.m_is_admin);
    q.bindNoCopy(3, user.get_name());
    q.bind(4, (void*)hp, 128);
    q.bindNoCopy(5, user.m_email);
    q.bindNoCopy(6, user.m_locale);
    q.bind(7, (int64_t) user.m_joined_ts);
    auto ret = q.exec();
    return ret;
}