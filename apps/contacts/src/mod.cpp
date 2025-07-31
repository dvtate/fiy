//
// Created by tate on 7/30/25.
//


#include "../../../modlib/fediymodpp.hpp"

#include "DB.hpp"
#include "Pages.hpp"
#include "Contact.hpp"

const fiy_host_info_t* g_host_info;

void handle_request(struct fiy_request_t* request, fiy_callback_t cb) {
    auto& req = *(fiy::Request*) request;

    // Everything here requires a login
    if (req.user == nullptr) {
        req.respond(cb, 401);
        return;
    }

    std::string_view path{req.path};

    if (path.starts_with("/main.css")) {
        static const char css_file[] = "/main.css";
        req.respond(cb, 200,
            Pages::file_contents<css_file>(),
            "Content-Type: text/css\nCache-Control: max-age=604800"
        );
        return;
    }

    if (path == "/" || (path.size() > 1 && path[1] == '?')) {

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
    g_host_info = host_info;
    return &mod_info;
}
