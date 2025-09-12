//
// Created by tate on 7/30/25.
//

#include <cstdint>
#include <string_view>
#include <utility>

#include "../../../modlib/fediymod.hpp"

#include "DB.hpp"
#include "Pages.hpp"
#include "Contact.hpp"

fiy::HostInfo g_host_info;

inline void get_profile(std::string_view user, fiy::Request& req, fiy_callback_t cb) {
    // TODO if local request, check and see if the user already has a contact for that user

    // Handle remote user
    auto at_idx = user.find('@');
    if (at_idx != std::string_view::npos) {
        if (at_idx > user.size() - 1) { // "test@"
            req.respond(cb, 400, "Invalid username");
            return;
        }

        auto dom = user.substr(at_idx + 1);
        if (dom == g_host_info.domain) {
            // Local user
            user = user.substr(0, at_idx);
            // fallthrough to get local user profile
        } else {
            // Remote user
            struct Ctx {
                fiy_callback_t cb;
                fiy::Request* req;
                std::string domain;
            };
            auto ctx = new Ctx{
                .cb=cb,
                .req=&req,
                .domain=std::string(dom)
            };
            req.domain = ctx->domain.c_str();

            g_host_info.request(
                "contacts",
                &req,
                ctx,
                [](const struct fiy_response_t* res, void* pctx){
                    auto* ctx = (struct Ctx*) pctx;

                    if (res == nullptr) {
                        // Failed
                        ctx->req->respond(ctx->cb, 404, "User not found at " + ctx->domain);
                    } else {
                        // Success, forward the response
                        ctx->req->respond(ctx->cb, *res);
                    }

                    delete ctx;
                }
            );
            return;
        }
    }

    auto card = DB::get_profile(std::string(user), req.user, req.domain);
    if (card.empty())
        req.respond(cb, 404, "Not found");
    else
        req.respond(cb, 200, card, "Content-Type:text/vcard");
}

void handle_request(struct fiy_request_t* request, fiy_callback_t cb) {
    auto& req = *(fiy::Request*) request;

    std::string_view path{req.path};

    // Get user profile
    if (req.method == (uint8_t) fiy::Request::Method::GET && path.starts_with("/profile/")) {
        path.remove_prefix(9);
        get_profile(path, req, cb);
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

    // Fontawesome fonts
    if (path == "/fa/fa.css") {
        static constexpr char file_path[] = "font-awesome.css";
        req.respond(cb, 200,
            Pages::file_contents<file_path>(),
            "\nContent-Type: text/css"
            "\nCache-Control: max-age=604800"
        );
    }
    if (path.starts_with("/fonts/fontawesome-webfont.")) {
        path.remove_prefix(27);
        if (path.starts_with("eot")) {
            static constexpr char file_path[] = "fontawesome-webfont.eot";
            req.respond(cb, 200,
                Pages::file_contents<file_path>(),
                "Content-Type: application/vnd.ms-fontobject\nCache-Control: max-age=604800"
            );
            return;
        }
        if (path.starts_with("woff2")) {
            static constexpr char file_path[] = "fontawesome-webfont.woff2";
            req.respond(cb, 200,
                Pages::file_contents<file_path>(),
                "Content-Type: font/woff2\nCache-Control: max-age=604800"
            );
            return;
        }
        if (path.starts_with("woff")) {
            static constexpr char file_path[] = "fontawesome-webfont.woff";
            req.respond(cb, 200,
                Pages::file_contents<file_path>(),
                "Content-Type: font/woff\nCache-Control: max-age=604800"
            );
            return;
        }
        if (path.starts_with("ttf")) {
            static constexpr char file_path[] = "fontawesome-webfont.ttf";
            req.respond(cb, 200,
                Pages::file_contents<file_path>(),
                "Content-Type: font/ttf\nCache-Control: max-age=604800"
            );
            return;
        }
        if (path.starts_with("svg")) {
            static constexpr char file_path[] = "fontawesome-webfont.svg";
            req.respond(cb, 200,
                Pages::file_contents<file_path>(),
                "Cache-Control: max-age=604800"
            );
            return;
        }
    }


    if (path == "/" || (path.size() > 1 && path[1] == '?')) {
        req.respond(cb, 200, Pages::index_html(), "Content-Type: text/html");
        return;
    }

    if (path.starts_with("/profile/")) {
        path.remove_prefix(9);
        get_profile(path, req, cb);
        return;
    }

    if (path == "/all") {
        req.respond(cb, 200, DB::get_user_rolodex(req.user), "Content-Type: text/vcard");
        return;
    }

    if (path == "/save") {
        VC card;
        card.parse(std::string(request->body, request->body_len));
        card.owner = req.user;
        switch (DB::save_contact(card)) {
            case DB::Success:
                req.respond(cb, 200, card.to_vcard(), "Content-Type: text/vcard");
                return;
            case DB::Error:
                req.respond(cb, 500, "Server Error");
                return;
            case DB::Unauthorized:
                req.respond(cb, 401, "Unauthorized");
                return;
        }
    }

    if (path.starts_with("/delete/")
        && req.method == (uint8_t) fiy::Request::DELETE
    ) {
        path.remove_prefix(7);

        // TODO
        req.respond(cb, 500, "TODO");
        return;
    }

    // Invalid path
    req.respond(cb, 404, "Not found");
}


extern "C" fiy_mod_info_t* start(const fiy_host_info_t* host_info) {
    static fiy_mod_info_t mod_info = {
        .on_request=handle_request,
        .on_peer_domain_changed=nullptr,
        .on_username_changed=nullptr
    };
    g_host_info = *host_info;
    std::cerr << "ghip: " <<g_host_info.data_dir <<std::endl;

    return &mod_info;
}
