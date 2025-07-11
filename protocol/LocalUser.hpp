#pragma once

#include <string>
#include <memory>
#include <deque>
#include <unordered_map>

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
        // TODO maybe track sessionId too?


        static std::string get_token_string() {
            // Create random generator that picks indices charset
            // https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
            static thread_local std::random_device rd;
            static thread_local std::mt19937 gen(rd());
            std::uniform_int_distribution<unsigned short> dist(1, 63);
            const char charset[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

            // Generate token using random chars from charset
            // Probably a better way to do this that doesn't use operator+=
            std::string ret;
            ret.reserve(TOKEN_LEN);
            for (int i = 0; i < TOKEN_LEN; i++)
                ret += charset[dist(gen)];
            return ret;
        }

        AuthToken(
            std::shared_ptr<LocalUser> user,
            std::string token = get_token_string(),
            time_t expiration = std::time(nullptr) + SESSION_LIFETIME
        ):
            m_user(std::move(user)),
            m_token(std::move(token)),
            m_expiration(expiration)
        {}

        [[nodiscard]] bool is_expired(const time_t now = std::time(nullptr)) const {
            return now > m_expiration;
        }

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
