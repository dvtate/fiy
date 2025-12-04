//
// Created by tate on 12/2/25.
//

#include <string_view>

#include "../../../modlib/fediymod.hpp"

#include "git_http_backend.hpp"
#include "Pages.hpp"

fiy::HostInfo g_host_info;


void repo_create_page(const fiy::Request& req, const fiy_callback_t cb) {
    std::string p = "<form method='POST'>"
                    ""
                    "</form>";
}

void repo_create_action(const fiy::Request& req, const fiy_callback_t cb) {

}

void unauthenticated(const fiy::Request& req, const fiy_callback_t cb) {
    // TODO either make a pretty page or redirect them to the portal/login page
    req.respond(cb, 401, "Unauthorized");
}

void handle_request(struct fiy_request_t* request, fiy_callback_t cb) {
    auto& req = *static_cast<fiy::Request*>(request);

    std::string_view path = req.path;

    if (path == "/") {
        static constexpr char file_path[] = "/landing.html";
        req.respond(cb, 200,
            Pages::file_contents<file_path>(),
            "Content-Type: text/html; charset=utf-8\nCache-Control: max-age=604800"
        );
        return;
    }

    if (path.starts_with("/repo")) {
        if (req.locality(g_host_info.domain) > fiy::Request::Locality::INSTANCE) {
            unauthenticated(req, cb);
            return;
        }
        path.remove_prefix(5);
        if (path.starts_with("/create")) {
            if (req.method == fiy::Request::Method::GET)
                repo_create_page(req, cb);
            else if (req.method == fiy::Request::Method::POST)
                repo_create_action(req, cb);
        }
        req.respond(cb, 404, "Not Found");
        return;
    }

    if (path.starts_with("/api")) {
        // TODO
        req.respond(cb, 404, "Not Found");
        return;
    }

    // else it's a repo

    git_repo_cgi(req, cb);
}

void delete_user(const char* username) {
    // TODO
}


extern "C" fiy_mod_info_t* start(const fiy_host_info_t* host_info) {
    static fiy_mod_info_t mod_info = {
        .on_request = handle_request,
        .delete_user = nullptr,
        .id="fiy.git",
        .version = "0.0"
    };
    g_host_info = *host_info;

    return &mod_info;
}
