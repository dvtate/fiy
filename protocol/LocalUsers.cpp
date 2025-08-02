//
// Created by tate on 7/8/25.
//

#include "App.hpp"

#include "LocalUsers.hpp"



LocalUsers::LocalUsers() {
    m_cron = std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(
                    std::chrono::seconds(60 * 5));
            this->cron();
        }
    });
}

std::shared_ptr<LocalUser> LocalUsers::get_username(const std::string& username) {
    RWMutex::LockForRead lock{m_mtx};

    // Use cache
    auto it = m_username_cache.find(username);
    if (it != m_username_cache.end())
        return it->second;

    // Use database
    auto user = DB::get_user(username);
    if (user == nullptr)
        return user;
    m_username_cache[username] = user;
    return user;
}

std::shared_ptr<LocalUser> LocalUsers::auth_user(const std::string& auth_token) {
    LocalUser::AuthToken token{nullptr, auth_token, 0};
    RWMutex::LockForRead lock{m_mtx};

    DEBUG_LOG("Checking token: " <<auth_token);
    auto it = m_token_cache.find(token);
    if (it == m_token_cache.end())
        return nullptr;

    // Cron should remove tokens when they become invalid
//        if (it->is_expired())
//            return nullptr;

    return it->m_user;
}

// Should run every 1-10 mins
void LocalUsers::cron() {
    RWMutex::LockForWrite lock{m_mtx};

    // Remove expired auth tokens
    std::erase_if(m_token_cache,
        [now = g_app->now()] (const LocalUser::AuthToken& t) {
            return t.is_expired(now);
        }
    );
}

void LocalUsers::deauth_token(const std::string& auth_token) {
    LocalUser::AuthToken token{nullptr, auth_token, 0};
    RWMutex::LockForWrite lock{m_mtx};
    m_token_cache.erase(token);
}

LocalUser::AuthToken LocalUsers::login_user(const std::string& username, const std::string& password) {
    // Get user from database
    auto user = DB::get_user(username, std::move(password));
    if (user == nullptr)
        return  LocalUser::AuthToken(nullptr, "", 0);

    // Get preliminary auth token
    // TODO maybe stay-logged-in option with longer duration?
    LocalUser::AuthToken token{user};

    RWMutex::LockForWrite lock{m_mtx};
    m_username_cache.emplace(username, user);

    // Generate auth token
    while (m_token_cache.contains(token))
        token.m_token = LocalUser::AuthToken::get_token_string();
    DEBUG_LOG("added token: " << token.m_token);
    m_token_cache.emplace(token);
    return token;
}

void LocalUsers::delete_user(const std::string& username) {
    delete_user(get_username(username));
}
void LocalUsers::delete_user(std::shared_ptr<LocalUser> user) {
    RWMutex::LockForWrite lock{m_mtx};
    std::erase_if(m_token_cache, [&user](const LocalUser::AuthToken& t) {
        return t.m_user == user;
    });

}