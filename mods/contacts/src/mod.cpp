//
// Created by tate on 7/30/25.
//

#include <cstdint>
#include <string_view>

#include "../../../modlib/fediymod.hpp"

#include "DB.hpp"
#include "Pages.hpp"
#include "Contact.hpp"

fiy::HostInfo g_host_info;

void handle_request(struct fiy_request_t* request, fiy_callback_t cb) {
    auto& req = *(fiy::Request*) request;

    std::string_view path{req.path};

    // Get user profile
    if (req.method == (uint8_t) fiy::Request::Method::GET && path.starts_with("/profile/")) {
        path.remove_prefix(9);
        // TODO split user on @ and if it's remote call remote server instead
        auto card = DB::get_profile(std::string(path), req.user, req.domain);
        if (card.empty())
            req.respond(cb, 401, "Not found");
        else
            req.respond(cb, 200, card, "Content-Type:text/vcard");
        return;
    }

    // Everything here requires a login
    if (req.user == nullptr) {
        static const fiy::Response no_auth_resp{
            303, nullptr, 0,
            "Location: " + g_host_info.host_base_uri() + "/portal/login"
        };
        req.respond(cb, no_auth_resp);
        return;
    }

    if (path.starts_with("/main.css")) {
        req.respond(cb, 200,
            Pages::main_css(),
            "Content-Type: text/css\nCache-Control: max-age=604800"
        );
        return;
    }
    if (path.starts_with("/main.js")) {
        req.respond(cb, 200,
            Pages::main_js(),
            "Content-Type: text/javascript\nCache-Control: max-age=604800"
        );
        return;
    }

    if (path == "/" || (path.size() > 1 && path[1] == '?')) {
        req.respond(cb, 200, Pages::index_html(), "Content-Type: text/html");
        return;
    }

    if (path.starts_with("/profile/")) {
        path.remove_prefix(9);

        // Get user components
        const auto iat = path.find('@');
        std::string_view p_user, p_domain;
        if (iat != std::string_view::npos) {
            p_user = path.substr(0, iat);
            p_domain = path.substr(iat+1);
        } else {
            p_user = path;
            p_domain = "";
        }





    }

//    - Add contact
//    - Edit contact
//    - Delete contact
//    - List contacts
//    - Search contacts
//    - Web interface + API
}


extern "C" fiy_mod_info_t* start(const fiy_host_info_t* host_info) {
    static fiy_mod_info_t mod_info = {
        .on_request=handle_request,
        .on_peer_domain_changed=nullptr,
        .on_username_changed=nullptr,
    };
    g_host_info = *host_info;
    return &mod_info;
}
