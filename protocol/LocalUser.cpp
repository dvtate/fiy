#include "nlohmann/json.hpp"

#include "../util/Crypto.hpp"

#include "LocalUser.hpp"

#include "App.hpp"

LocalUser::AuthToken::AuthToken(std::shared_ptr<LocalUser> user, std::string token,
                                time_t expiration)
    : m_user(std::move(user)), m_token(std::move(token)), m_expiration(expiration) {}

LocalUser::AuthToken::AuthToken(std::shared_ptr<LocalUser> user, std::string token)
    : LocalUser::AuthToken(std::move(user), std::move(token), g_app->now() + SESSION_LIFETIME) {}

LocalUser::AuthToken::AuthToken(std::shared_ptr<LocalUser> user)
    : LocalUser::AuthToken(std::move(user),
                           Crypto::get_token_string<LocalUser::AuthToken::TOKEN_LEN>(),
                           g_app->now() + SESSION_LIFETIME) {}

bool LocalUser::AuthToken::is_expired() {
    return is_expired(g_app->now());
}

std::string LocalUser::json() const {
    nlohmann::json ret = {{"name", m_name},   {"username", m_username}, {"isAdmin", m_is_admin},
                          {"email", m_email}, {"locale", m_locale},     {"joinTs", m_joined_ts}};
    return ret.dump();
}