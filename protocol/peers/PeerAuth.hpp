//
// Created by tate on 5/5/26.
//

#pragma once

#include <ctime>

#include <string>
#include <random>

#include "../../util/Crypto.hpp"

struct PeerAuth {
    /// Super secret symmetric key
    std::string sym_key;

    /// Public key
    std::string pubkey;

    /// Bearer token for authenticating endpoints
    std::string bearer_token_we_send;
    std::string bearer_token_we_accept;

    /// When was this authentication credential created?
    time_t link_ts = 0;

    static constexpr time_t SESSION_LIFETIME = 60 * 60 * 24 * 7; // 1 week
    static constexpr time_t MAX_RESPONSE_TIME = 60 * 90; // 1.5 hrs timeout
    static constexpr int TOKEN_LEN = 24;
    static constexpr unsigned REAUTH_HTTP_STATUS = 478;

    static std::string get_token_string() {
        return Crypto::get_token_string<TOKEN_LEN>();
    }

    PeerAuth() = default;

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

    /**
     *
     * @param our_generated_token bearer token that we accept from peers
     *      should come from g_fiy->peers.new_token_stub()
     * @param our_symkey_half our half of the shared secret
     */
    void handshake_start(
        const std::string& our_generated_token,
        const std::string& peer_pubkey = {},
        const std::string& our_symkey_half = get_token_string()
    ) {
        this->pubkey = peer_pubkey;

        bearer_token_we_send = our_generated_token;

        // Generate our half of the symkey
        sym_key = our_symkey_half;
    }
    std::string handshake_request_body() const;
    bool handshake_parse_response(const std::string_view body);
    void handshake_end();

    [[nodiscard]] bool is_expired() const;
    [[nodiscard]] bool is_expired(const time_t now) const {
        return now > (link_ts + SESSION_LIFETIME);
    }
};
