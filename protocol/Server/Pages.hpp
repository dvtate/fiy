//
// Created by tate on 4/10/26.
//

#pragma once

#include "../../util/FileCache.hpp"

#include "../FIY.hpp"

#include "Session.hpp"

inline std::string fiy_portal_templates_dir() {
    return g_fiy->config.data_dir + "/pages/";
}

struct Pages : public FileCache<fiy_portal_templates_dir> {

    static ReplacementMap get_host_data() {
        return {
                { "{{domain}}",     g_fiy->config.hostname  },
                { "{{protocol}}",   "https://"      },
            };
    }

    /// Automatically replace host_data in the loaded template
    template<const char* FileSubPath>
    static const std::string& file_contents() {
        return FileCache<fiy_portal_templates_dir>::file_contents<FileSubPath>(get_host_data());
    }

    static Session::StringResponse signup_page(unsigned status = 200, const std::string err = "") {
        static const char path[] = "signup.html";
        Session::StringResponse res;
        res.result(status);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.body() = mustache(file_contents<path>(),
            ReplacementMap{ { "fail_reason", err } } );
        return res;
    }
    static Session::StringResponse login_page(unsigned status = 200, const std::string err = "") {
        static const char path[] = "login.html";
        Session::StringResponse res;
        res.result(status);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.body() = mustache(file_contents<path>(),
            ReplacementMap{ { "fail_reason", err } } );
        return res;
    }

    static Session::StringResponse portal_apps(const LocalUser& user) {
        static const char path[] = "home.html";
        Session::StringResponse res;
        res.result(200);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.body() = mustache(file_contents<path>(),
            ReplacementMap{
                { "user_data", user.json() },
                { "installed_apps", g_fiy->mods.get_mods_json() }
            });
        return res;
    }

    static std::string portal_settings(const LocalUser& user) {
        static const char path[] = "settings.html";
        return mustache(file_contents<path>(),
            ReplacementMap{
                { "user_data", user.json() },
            });
    }
};
