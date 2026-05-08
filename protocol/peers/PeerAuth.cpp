//
// Created by tate on 5/5/26.
//

#include "PeerAuth.hpp"

#include "../../util/FileCache.hpp"

#include "../FIY.hpp"

PeerAuth::PeerAuth(
    std::string sym_key,
    std::string peer_provided_token,
    std::string our_generated_token
):
    PeerAuth(
        std::move(peer_provided_token),
        std::move(our_generated_token),
        std::move(sym_key),
        g_fiy->now()
    )
{}

bool PeerAuth::is_expired() const {
    return is_expired(g_fiy->now());
}

/**
 *
 * @return
 */
std::string PeerAuth::handshake_request_body() const {
    // TODO encryption
    std::string payload = concat(
        g_fiy->config.hostname,
        '\n',
        this->sym_key, // partial symkey
        '\n',
        this->bearer_token_we_accept
    );
    const auto signature = Crypto::SSL::sign(g_fiy->config.private_key, payload);
    payload += '\n' + signature;
    return payload;
}

bool PeerAuth::handshake_parse_response(const std::string_view body) {
    // TODO encryption

    auto eol = body.find('\n');
    if (eol == std::string::npos) {
        LOG_ERR("PeerAuth: Invalid handshake response: no bearer token");
        return false;
    }

    this->bearer_token_we_send = body.substr(0, eol);
    auto start = eol + 1;
    eol = body.find('\n', start);
    if (eol == std::string::npos) {
        LOG_ERR("PeerAuth: Invalid handshake response: no secret");
        return false;
    }

    const auto secret_part2 = body.substr(start, eol - start);
    this->sym_key += secret_part2;
    const auto sig = std::string(body.substr(eol + 1));
    const auto pre_sig = concat("{}\n{}",
        this->bearer_token_we_send, secret_part2);
    if (!Crypto::SSL::verify(this->pubkey, pre_sig, sig)) {
        LOG_ERR("PeerAuth: Handshake response has invalid signature");
        return false;
    }

    this->link_ts = g_fiy->now();
    return true;
}
