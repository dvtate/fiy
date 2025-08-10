#pragma once

#include <cstring>
#include <ctime>
#include <memory>
#include <random>
#include <string>

#include "../util/Crypto.hpp"

class PeerAuth {
public:
    /// Super secret symmetric key
    std::string m_sym_key;

    /// Public key
    // dvtt: should we store this for later in case they change it?
    //    std::string m_pubkey;

    /// Bearer token for authenticating endpoints
    std::string m_bearer_token_we_send;
    std::string m_bearer_token_we_accept;

    /// When do we need to refresh auth credentials?
    // dvtt: why bother refreshing?
    time_t m_expire_ts;

    static constexpr time_t SESSION_LIFETIME = 60 * 60 * 24 * 7;  // 1 week
    static constexpr int TOKEN_LEN = 24;

    static std::string get_token_string() {
        return Crypto::get_token_string<TOKEN_LEN>();
    }

    PeerAuth(std::string sym_key,
             //        std::string pubkey,
             std::string peer_provided_token, std::string our_generated_token,
             const time_t expire_ts)
        : m_sym_key(std::move(sym_key)),
          //        m_pubkey(std::move(pubkey)),
          m_bearer_token_we_send(std::move(peer_provided_token)),
          m_bearer_token_we_accept(std::move(our_generated_token)),
          m_expire_ts(expire_ts) {}

    PeerAuth(std::string sym_key,
             //        std::string pubkey,
             std::string peer_provided_token, std::string our_generated_token = get_token_string());

    [[nodiscard]] bool is_expired(const time_t now) const {
        return now > m_expire_ts;
    }
    [[nodiscard]] bool is_expired() const;
};

// This is another server on a different domain
class Peer {
public:
    PeerAuth m_auth;
    char* m_domain;

    Peer(const std::string& domain, PeerAuth auth) : m_auth(std::move(auth)) {
        m_domain = (char*)malloc(domain.size() + 1);
        strcpy(m_domain, domain.c_str());
    }
    Peer(char* domain, PeerAuth auth) : m_auth(std::move(auth)), m_domain(domain) {}

    ~Peer() {
        free(m_domain);
    }

    [[nodiscard]] std::string sig(const std::string& appid, const std::string& path,
                                  const std::string& user, const std::size_t body_len,
                                  const std::string& ts_str) const;
};
