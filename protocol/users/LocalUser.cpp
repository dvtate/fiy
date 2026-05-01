#include "LocalUser.hpp"

#include <nlohmann/json.hpp>

#include "../../util/Crypto.hpp"

#include "../FIY.hpp"

std::string LocalUser::json() const {
    nlohmann::json ret = {
        { "username", username },
        { "isAdmin", is_admin },
        { "email", email },
        { "joinTs", joined_ts }
    };
    return ret.dump();
}

/**
 * Get the password hash
 * @param password user-provided raw password (should be std::moved in)
 * @param buff buffer to hold the hashed password
 * @return pointer to buff that now contains hashed password
 * @remark should std::move password to this function
 */
unsigned char* LocalUser::hash_password(
    std::string password,
    unsigned char (&buff)[PASSWORD_HASH_SIZE]
) {
    // Hash provided password
    // TODO switch to argon2 or something better
    static_assert(PASSWORD_HASH_SIZE == SHA512_DIGEST_LENGTH);
    password += g_fiy->config.salt;
    return SHA512(reinterpret_cast<unsigned char*>(password.data()), password.size(), buff);
}

/**
 *
 * @param provided_password user-provided password
 * @param db_hp password from the database
 * @return true if the password is correct
 */
bool LocalUser::check_password_hash(std::string provided_password, const std::string_view db_hp) {
    // Verify right size
    static_assert(PASSWORD_HASH_SIZE == SHA512_DIGEST_LENGTH);
    if (db_hp.size() < SHA512_DIGEST_LENGTH) {
        DEBUG_LOG("Invalid password hash");
        return false;
    }

    // Get hash buffers
    unsigned char buff[PASSWORD_HASH_SIZE];
    const auto* hp = hash_password(std::move(provided_password), buff);
    const auto* ref = reinterpret_cast<const unsigned char*>(db_hp.data());

    // Verify that password is correct
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
        if (hp[i] != ref[i]) {
            // DEBUG_LOG(i << ":  '" << (unsigned)hp[i] << "' != '" << (unsigned)ref[i] << "'");
            return false; // wrong pw
        }
    return true;
}

LocalUser::AuthToken::AuthToken(
    std::shared_ptr<LocalUser> user,
    std::string token,
    const time_t expiration
):
    m_user(std::move(user)),
    m_token(std::move(token)),
    m_expiration(expiration)
{}

LocalUser::AuthToken::AuthToken(std::shared_ptr<LocalUser> user, std::string token):
    LocalUser::AuthToken(
        std::move(user),
        std::move(token),
        g_fiy->now() + SESSION_LIFETIME
    )
{}

LocalUser::AuthToken::AuthToken(std::shared_ptr<LocalUser> user):
    LocalUser::AuthToken(
        std::move(user),
        get_token_string(),
        g_fiy->now() + SESSION_LIFETIME
    )
{}

bool LocalUser::AuthToken::is_expired() const {
    return is_expired(g_fiy->now());
}
