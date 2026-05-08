#pragma once

#include <string>
#include <memory>
#include <ctime>
#include <cstring>

#include "PeerAuth.hpp"

/// This is another FIY server on a different domain
struct Peer {
    /// Domain to access the peer
    char* domain = nullptr;

    /// Peer authentication and handshake data
    PeerAuth auth{};

    explicit Peer(const char* domain, PeerAuth peer_auth = {}):
        domain(strdup(domain)), auth(std::move(peer_auth))
    {}

    explicit Peer(const std::string_view domain, PeerAuth peer_auth = {}):
        domain(strndup(domain.data(), domain.size())),
        auth(std::move(peer_auth))
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

    static std::shared_ptr<Peer> parse_handshake_request(
        std::string_view body,
        std::string& signature
    );
};

