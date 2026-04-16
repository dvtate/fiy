#pragma once

#include <string>
#include <memory>
#include <ctime>
#include <cstring>
#include <random>

#include "../../util/Crypto.hpp"

struct PeerAuth {
    /// Super secret symmetric key
    std::string sym_key;

    /// Public key
    // Should we store this for later in case they change it?
//    std::string pubkey;

    /// Bearer token for authenticating endpoints
    std::string bearer_token_we_send;
    std::string bearer_token_we_accept;

    /// When was this authentication credential created?
    time_t link_ts;

    static constexpr time_t SESSION_LIFETIME = 60 * 60 * 24 * 7; // 1 week
    static constexpr int TOKEN_LEN = 24;

    static std::string get_token_string() {
        return Crypto::get_token_string<TOKEN_LEN>();
    }

    PeerAuth(
        std::string sym_key,
//        std::string pubkey,
        std::string peer_provided_token,
        std::string our_generated_token,
        const time_t now
    ):
        sym_key(std::move(sym_key)),
//        m_pubkey(std::move(pubkey)),
        bearer_token_we_send(std::move(peer_provided_token)),
        bearer_token_we_accept(std::move(our_generated_token)),
        link_ts(now)
    {}

    PeerAuth(
        std::string sym_key,
//        std::string pubkey,
        std::string peer_provided_token,
        std::string our_generated_token = get_token_string()
    );

    [[nodiscard]] bool is_expired() const;
    [[nodiscard]] bool is_expired(const time_t now) const {
        return now > (link_ts + SESSION_LIFETIME);
    }
};

// This is another server on a different domain
struct Peer {
    PeerAuth auth;
    char* domain;

    Peer(const std::string& hostname, PeerAuth peer_auth):
        auth(std::move(peer_auth))
    {
        domain = (char*) malloc(hostname.size() + 1);
        strcpy(domain, hostname.c_str());
    }
    Peer(char* domain, PeerAuth peer_auth):
        auth(std::move(peer_auth)), domain(domain)
    {}

    ~Peer() {
        free(domain);
    }

    [[nodiscard]] std::string sig(
        const std::string& appid,
        const std::string& path,
        const std::string& user,
        const std::size_t body_len,
        const std::string& ts_str
    ) const;
};

