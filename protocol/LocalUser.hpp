#pragma once

#include <string>
#include <memory>
#include <deque>
#include <unordered_map>

#include "../util/Crypto.hpp"

#include "Peer.hpp"


/**
 * User authenticated on our instance (not remotee)
 */
class LocalUser {
    std::string m_name;
    // TODO how do we handle updating users safely?
    std::string m_username;
public:
    bool m_is_admin{false};
    std::string m_email;
    std::string m_locale; // TODO replace this with something better?
    time_t m_joined_ts{0};
    std::string m_about;

    LocalUser(std::string username, bool is_admin):
        m_username(std::move(username)),
        m_is_admin(is_admin)
    {}

    LocalUser(
        std::string name,
        std::string username,
        bool is_admin,
        std::string email,
        std::string locale,
        time_t joined_ts,
        std::string about = ""
    ):
        m_name(std::move(name)),
        m_username(std::move(username)),
        m_is_admin(is_admin),
        m_email(std::move(email)),
        m_locale(std::move(locale)),
        m_joined_ts(joined_ts),
        m_about(std::move(about))
    {}

    /**
     * Update user's username
     * @param name
     * @return nullptr on success, reason string on fail
     */
    const char* set_name(std::string name) {
        if (name.size() > 128)
            return "Name must be less than 128 characters";
        m_name = std::move(name);
        return nullptr;
    }
    [[nodiscard]] const std::string& get_name() const {
        return m_name.empty() ? m_username : m_name;
    }

    /**
     * Update user's username
     * @param username
     * @return nullptr on success, reason string on fail
     */
    const char* set_username(std::string username) {
        if (username.size() > 32)
            return "Username must be less than 32 characters";
        for (auto c: username)
            if (!isalnum(c))
                return "Username must have only alphanumeric characters";
        m_username = std::move(username);
        // TODO notify peers & apps
        return nullptr;
    }

    [[nodiscard]] const std::string& get_username() const {
        return m_username;
    }

    bool write_changes_to_db();

    [[nodiscard]] std::string json() const;

//    static std::string login(const std::string& username, const std::string& password);

    class AuthToken {
    public:
        static constexpr time_t SESSION_LIFETIME = 60 * 60 * 24 * 14; // 2 weeks
        static constexpr int TOKEN_LEN = 24;

        std::shared_ptr<LocalUser> m_user;
        std::string m_token;
        time_t m_expiration;

        static std::string get_token_string() {
            return Crypto::get_token_string<TOKEN_LEN>();
        }

        AuthToken(std::shared_ptr<LocalUser> user, std::string token, std::time_t expiration);
        AuthToken(std::shared_ptr<LocalUser> user, std::string token);
        explicit AuthToken(std::shared_ptr<LocalUser> user);

        [[nodiscard]] bool is_expired(const std::time_t now) const {
            return now > m_expiration;
        }
        bool is_expired();

        // These are for storing in stl containers... ugly solution
        struct Hash {
            std::size_t operator()(const AuthToken& token) const {
                return std::hash<std::string>{}(token.m_token);
            }
            std::size_t operator()(const AuthToken* token) const {
                return std::hash<std::string>{}(token->m_token);
            }
        };
        struct TokenEquals {
            inline bool operator()(const AuthToken& a, const AuthToken& b) const {
                return a.m_token == b.m_token;
            };
            inline bool operator()(const AuthToken* a, const AuthToken* b) const {
                return a->m_token == b->m_token;
            };
        };
        struct Compare {
            inline bool operator()(const AuthToken& a, const AuthToken& b) const {
                return a.m_token < b.m_token;
            };
            inline bool operator()(const AuthToken* a, const AuthToken* b) const {
                return a->m_token < b->m_token;
            };
        };
    };
};
