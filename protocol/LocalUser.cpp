#include "nlohmann/json.hpp"

#include "LocalUser.hpp"

#include "App.hpp"


std::string LocalUser::json() const {
    nlohmann::json ret = {
        { "name", m_name },
        { "username", m_username },
        { "isAdmin", m_is_admin },
        { "email", m_email },
        { "locale", m_locale },
        { "joinTs", m_joined_ts }
    };
    return ret.dump();
}