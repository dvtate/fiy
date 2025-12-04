//
// Created by tate on 6/4/24.
//

#pragma once

#include <string>

#include "../third_party/Mustache/mustache.hpp"

#include "../util/FileCache.hpp"

// TODO i18n
// TODO minification
// dvtt: keeping this as an instanced object to make it easier to dynamically change locale

class LocalUser;

extern std::string fiy_templates_dir();

/**
 * Handles server-side rendering of pages via templating engine
 */
class Pages : public FileCache<fiy_templates_dir> {
    kainjow::mustache::mustache
        m_portal_apps_template,
        m_portal_settings_template,
        m_login_template,
        m_signup_template;

public:
    Pages();

    std::string login_page(const std::string& fail_reason = "");
    std::string signup_page(const std::string& fail_reason = "");

    std::string portal_apps(const LocalUser& user);
    std::string portal_settings(const LocalUser& user);


protected:
    static kainjow::mustache::mustache open_mustache_file(std::string&& path);

};
