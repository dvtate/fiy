//
// Created by tate on 6/4/24.
//

#include <fstream>

#include "nlohmann/json.hpp"

#include "App.hpp"

#include "Pages.hpp"

using namespace kainjow;

std::string Pages::load_file_as_string(std::string&& path) {
    // Open the file
    std::ifstream f{path};
    if (!f.is_open()) {
        LOG("Error: Could not open file " << path);
        return ""; // Return an empty string on failure
    }

    // Create a string big enough for the file
    std::string ret;
    f.seekg(0, std::ios::end);
    ret.reserve(1 + (ssize_t) f.tellg());
    f.seekg(0, std::ios::beg);

    // Read entire file content into return string
    ret.assign(std::istreambuf_iterator<char>(f),
               std::istreambuf_iterator<char>());
    return ret;
}

mustache::mustache Pages::open_mustache_file(std::string&& path) {
    std::string ret = Pages::load_file_as_string(std::move(path));
    // TODO fix or replace mustache instead of this
    return std::regex_replace(ret, std::regex("\\{\\{domain\\}\\}"), g_app->m_config.m_hostname);
    return ret;
}

std::string Pages::template_dir() {
    return g_app->m_config.m_data_dir + "/page_templates/";
}

Pages::Pages() {
    mustache::data global_data;
    global_data.set("domain", g_app->m_config.m_hostname);

    // Load templates with global data already filled in
    const auto dir = template_dir();
    m_portal_apps_template = open_mustache_file(dir + "home.html");//.render(global_data);
    m_portal_settings_template = open_mustache_file(dir + "settings.html");//.render(global_data);
    m_login_template = open_mustache_file(dir + "login.html");//.render(global_data);
    m_signup_template = open_mustache_file(dir + "signup.html");//.render(global_data);
}

std::string Pages::login_page(const std::string& fail_reason) {
    return m_login_template.render({ "fail_reason", fail_reason });
}

std::string Pages::signup_page(const std::string& fail_reason) {
    return m_signup_template.render({ "fail_reason", fail_reason });
}

std::string Pages::portal_apps(const LocalUser& user) {
    mustache::data d;
    d.set("user_data", user.json());
    d.set("installed_apps", g_app->m_mods.get_mods_json());
    return m_portal_apps_template.render(d);
}

std::string Pages::portal_settings(const LocalUser& user) {
    mustache::data d;
    d.set("user_data", user.json());
    return m_portal_settings_template.render({});
}