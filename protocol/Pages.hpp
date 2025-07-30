//
// Created by tate on 6/4/24.
//

#pragma once

#include <string>

#include "../third_party/Mustache/mustache.hpp"

// TODO i18n
// TODO minification

/**
 * Handles server-side rendering of pages via templating engine
 */
class Pages {
    kainjow::mustache::mustache
        m_portal_apps_template,
        m_portal_settings_template,
        m_login_template,
        m_signup_template;

public:
    Pages();

    static std::string template_dir();

    std::string login_page(const std::string& fail_reason = "");
    std::string signup_page(const std::string& fail_reason = "");

    std::string portal_apps(const LocalUser& user);
    std::string portal_settings(const LocalUser& user);


protected:
    static kainjow::mustache::mustache open_mustache_file(std::string&& path);

public:
    static std::string load_file_as_string(std::string&& file_path);

    /// Get cached contents of file as a string
    template<const char* FileSubPath>
    static const std::string& file_contents() {
        static const std::string contents = load_file_as_string(template_dir() + FileSubPath);
        return contents;
    }
};
