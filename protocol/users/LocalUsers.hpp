//
// Created by tate on 7/8/25.
//

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <chrono>

// #include <boost/unordered/unordered_flat_set.hpp>

#include "../../util/RWMutex.hpp"

#include "LocalUser.hpp"

class LocalUsers {
private:
    /**
     * Map of usernames -> users
     */
    // TODO can probably switch to unique_ptr
    // TODO use boost::unordered_flat_map
    std::unordered_map<std::string, std::shared_ptr<LocalUser>> m_username_cache;

    /**
     * Map of auth tokens -> users
     *
     * Using set instead of map to save memory
     */
    std::unordered_set<
        LocalUser::AuthToken,
        LocalUser::AuthToken::Hash,
        LocalUser::AuthToken::TokenEquals
    > m_token_cache;

    /**
     * Periodically filters out expired auth tokens
     */
    std::thread m_cron;

    /**
     * Thread safety
     */
    // TODO maybe one lock per collection instead?
    // TODO don't use RWMutex
    RWMutex m_mtx;

public:
    LocalUsers();

    void cron();

    bool add_user(const LocalUser& user, std::string password);
    std::shared_ptr<LocalUser> get_user(const std::string& username);
    static int auth_user(const std::string& username, std::string password);
    std::shared_ptr<LocalUser> auth_user(const std::string& auth_token);
    LocalUser::AuthToken login_user(const std::string& username, std::string password);
    void deauth_token(const std::string& auth_token);
    void delete_user(const std::string& username);

private:
    LocalUser::AuthToken authorize_user(std::shared_ptr<LocalUser> user);
};
