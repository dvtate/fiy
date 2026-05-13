#pragma once

#include <string>
#include <memory>
#include <deque>
#include <unordered_map>
#include <openssl/sha.h>

#include "../../third_party/SQLiteCpp/include/SQLiteCpp/Column.h"

#include "../../util/Crypto.hpp"

#include "../peers/Peer.hpp"

/**
 * User authenticated on our instance (not remotee)
 */
class LocalUser {
public:
    const std::string username;
    bool is_admin{false};

    // TODO leave these in the DB and fetch them as needed
    std::string email;
    time_t joined_ts{0};

    static constexpr size_t USERNAME_MAX_LENGTH = 32;
    static constexpr size_t NAME_MAX_LENGTH = 128;
    static constexpr size_t LOCALE_MAX_LENGTH = 12;
    static constexpr size_t EMAIL_MAX_LENGTH = 255;

    LocalUser(std::string username, const bool is_admin):
        username(std::move(username)),
        is_admin(is_admin)
    {}

    LocalUser(
        std::string username,
        const bool is_admin,
        std::string email,
        const time_t joined_ts
    ):
        username(std::move(username)),
        is_admin(is_admin),
        email(std::move(email)),
        joined_ts(joined_ts)
    {}

    [[nodiscard]] const std::string& get_username() const {
        return username;
    }
    [[nodiscard]] std::string json() const;

    static constexpr size_t PASSWORD_HASH_SIZE = SHA512_DIGEST_LENGTH;
    static unsigned char* hash_password(std::string password, unsigned char (&buff)[PASSWORD_HASH_SIZE]);
    static bool check_password_hash(std::string provided_password, std::string_view db_hp);
    static bool check_password_hash(std::string provided_password, const SQLite::Column& col) {
        return check_password_hash(
            std::move(provided_password),
            std::string_view(
                static_cast<const char*>(col.getBlob()),
                col.getBytes()
            )
        );
    }

    // TODO this system is too simple, loose session token and you're fucked
    // TODO this should be moved to its own class+file
    class AuthToken {
    public:
        static constexpr time_t SESSION_LIFETIME = 60 * 60 * 24 * 14; // 2 weeks
        static constexpr int TOKEN_LEN = 24;

        std::shared_ptr<LocalUser> m_user = nullptr;
        std::string m_token;
        time_t m_expiration = 0;
        // TODO maybe should add context (eg, IP address)

        static std::string get_token_string() {
            return Crypto::get_token_string<TOKEN_LEN>();
        }

        AuthToken(std::shared_ptr<LocalUser> user, std::string token, std::time_t expiration);
        AuthToken(std::shared_ptr<LocalUser> user, std::string token);
        explicit AuthToken(std::shared_ptr<LocalUser> user);
        AuthToken() = default; // Unauthenticated

        [[nodiscard]] bool is_expired(const std::time_t now) const {
            return now > m_expiration;
        }
        [[nodiscard]] bool is_expired() const;
        [[nodiscard]] bool is_valid() const {
            return m_expiration != 0;
        }

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
            bool operator()(const AuthToken& a, const AuthToken& b) const {
                return a.m_token == b.m_token;
            }
            bool operator()(const AuthToken* a, const AuthToken* b) const {
                return a->m_token == b->m_token;
            }
        };
        struct Compare {
            bool operator()(const AuthToken& a, const AuthToken& b) const {
                return a.m_token < b.m_token;
            }
            bool operator()(const AuthToken* a, const AuthToken* b) const {
                return a->m_token < b->m_token;
            }
        };
    };
};
