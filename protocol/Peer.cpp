//
// Created by tate on 7/31/25.
//
#include "App.hpp"

#include "Peer.hpp"

PeerAuth::PeerAuth(std::string sym_key, std::string peer_provided_token,
                   std::string our_generated_token)
    : PeerAuth(std::move(sym_key), std::move(peer_provided_token), std::move(our_generated_token),
               g_app->now() + SESSION_LIFETIME) {}

bool PeerAuth::is_expired() const {
    return is_expired(g_app->now());
}

std::string Peer::sig(const std::string& appid, const std::string& path, const std::string& user,
                      const std::size_t body_len,
                      const std::string& ts_str = std::to_string(g_app->now())) const {
    return Crypto::hmac(m_auth.m_sym_key, std::string("FIY") + appid + path + user +
                                              std::to_string(body_len) + ts_str);
}
