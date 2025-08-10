//
// Created by tate on 7/8/25.
//

#pragma once

#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "../util/RWMutex.hpp"

#include "LocalUser.hpp"

class LocalUsers {
private:
    /**
     * Map of usernames -> users
     */
    // TODO can probably switch to unique_ptr
    std::unordered_map<std::string, std::shared_ptr<LocalUser>> m_username_cache;

    /**
     * Map of auth tokens -> users
     *
     * Using set instead of map to save memory
     */
    std::unordered_set<LocalUser::AuthToken, LocalUser::AuthToken::Hash,
                       LocalUser::AuthToken::TokenEquals>
        m_token_cache;

    /**
     * Periodically filters out expired auth tokens
     */
    std::thread m_cron;

    /**
     * Thread safety
     */
    // TODO maybe one lock per collection instead?
    RWMutex m_mtx;

public:
    LocalUsers();

    std::shared_ptr<LocalUser> get_username(const std::string& username);
    std::shared_ptr<LocalUser> auth_user(const std::string& auth_token);
    LocalUser::AuthToken login_user(const std::string& username, const std::string& password);

    void cron();

    void deauth_token(const std::string& auth_token);
    void delete_user(const std::string& username);
    void delete_user(std::shared_ptr<LocalUser> username);
};
