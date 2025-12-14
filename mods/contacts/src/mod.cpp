//
// Created by tate on 7/30/25.
//

#include <cstdint>
#include <string_view>
#include <utility>

#include "../../../third_party/base64.hpp"

#include "../../../modlib/fediymod.hpp"

#include "DB.hpp"
#include "Pages.hpp"
#include "Contact.hpp"
#include "timezones.hpp"

fiy::HostInfo g_host_info;

/**
 * User profile endpoint
 * @param user_str user to get profile of
 * @param req request for profile
 * @param cb request callback
 */
static void get_profile(std::string_view user_str, fiy::Request& req, fiy::Callback cb) {
    // TODO if local request, check and see if the user already has a contact for that user

    const auto [user, dom] = g_host_info.split_user_str(user_str);

    // Handle local user
    if (dom.empty()) {
        const auto card = DB::get_profile(std::string(user), req.user, req.domain);
        if (card.empty())
            req.respond(cb, 404, "", fiy::Body("Not found"));
        else
            req.respond(cb, 200, "Content-Type:text/vcard", fiy::Body(card));
        return;
    }

    // Handle remote user
    struct Ctx {
        fiy::Callback cb;
        fiy::Request* req;
        std::string domain;
    };
    auto* ctx = new Ctx{
        .cb=cb,
        .req=&req,
        .domain=std::string(dom)
    };
    req.domain = ctx->domain.c_str();

    g_host_info.request(
        "contacts",
        &req,
        ctx,
        [](const struct fiy::fiy_response_t* res, void* pctx){
            auto* ctx = (struct Ctx*) pctx;

            if (res == nullptr || res->status < 0) {
                // Failed
                std::string body = "Failed to get profile from " + ctx->domain + ": ";
                if (res != nullptr)
                    body += fiy::Body::to_string(res->body);
                ctx->req->respond(ctx->cb,
                    500,
                    "Content-Type: text/html",
                    fiy::Body(body)
                );
            } else {
                // Success, forward the response
                ctx->req->respond(ctx->cb, *res);
            }

            delete ctx;
        }
    );
}

/**
 * Profile picture endpoint
 * @param user_str relevant user string
 * @param req request
 * @param cb request callback
 */
static void get_pfp(std::string_view user_str, fiy::Request& req, fiy::Callback cb) {
    auto [user, dom] = g_host_info.split_user_str(user_str);

    static const fiy::fiy_response_t default_pfp {
        .status = 404,
        .headers = "Content-Type: image/png\nCache-Control: max-age=300",
        .body = fiy::Body(
            (const char*)VC::default_pfp_raw,
            sizeof VC::default_pfp_raw
        )
    };

    // Local request
    const auto pfp_dataurl = DB::get_pfp(std::string(user),
        req.user,
        req.domain
    );
    // Either:
    // - dataurl: data:image/png;base64,data
    // - url: http(s) ...
    // - not found: "" empty string

    if (pfp_dataurl.empty()) {
        // Local user has no profile photo
        if (dom.empty()) {
            req.respond(cb, default_pfp);
            return;
        }

        // Remote user, with no locally overridden profile picture
        //  -> forward request to remote server

        struct Ctx {
            fiy::Callback cb;
            fiy::Request* req;
            std::string domain;
        };
        auto* ctx = new Ctx{
            .cb=cb,
            .req=&req,
            .domain=std::string(dom)
        };
        req.domain = ctx->domain.c_str();

        g_host_info.request(
            "contacts",
            &req,
            ctx,
            [](const struct fiy::fiy_response_t* res, void* pctx){
                auto* ctx = (struct Ctx*) pctx;

                if (!res || res->status < 0) {
                    // Failed
                    ctx->req->respond(ctx->cb, default_pfp);
                } else {
                    // Success, forward the response
                    ctx->req->respond(ctx->cb, *res);
                }

                delete ctx;
            }
        );
        return;
    }

    // Redirect
    if (pfp_dataurl.starts_with("http")) {
        req.respond(cb, 307, "Location: " + pfp_dataurl, fiy::Body());
    }

    if (!pfp_dataurl.starts_with("data:")) {
        g_host_info.log_warning("PFP invalid PHOTO property, not a dataurl");
        req.respond(cb, default_pfp);
        return;
    }
    std::string_view pfp = pfp_dataurl;
    pfp.remove_prefix(5);

    const auto end_type = pfp.find(';');
    const auto start_data = pfp.find(',');
    if (start_data == std::string::npos) {
        g_host_info.log_warning("PFP invalid PHOTO property");
        req.respond(cb, default_pfp);
        return;
    }

    std::string_view media_type;
    if (end_type == std::string_view::npos) {
        g_host_info.log_warning("PFP invalid PHOTO property, dataurl missing media-type");
        media_type = "image";
    }
    media_type = pfp.substr(0, std::min(end_type, start_data));

    const auto data = pfp.substr(start_data + 1);
    const auto raw_data = base64::decode_into<std::string>(data);

    std::string headers = "Cache-Control: max-age=300\nContentType: ";
    headers += media_type;
    req.respond(cb, 200, headers, fiy::Body(raw_data));
}

static void handle_request(struct fiy::fiy_request_t* request, fiy::Callback cb) {
    auto& req = *(fiy::Request*) request;

    std::string_view path{req.path};

    // Get user profile
    if (req.method == (uint8_t) fiy::Request::Method::GET && path.starts_with("/profile/")) {
        path.remove_prefix(9);
        get_profile(path, req, cb);
        return;
    }

    // Get user pfp
    if (path.starts_with("/pfp/")) {
        path.remove_prefix(5);
        get_pfp(path, req, cb);
        return;
    }

    if (path == "/tzdb") {
        // TODO 30 mins cache
        static const std::string tzdb_json = get_timezones_json();
        static const fiy::fiy_response_t tzdb_json_resp{
            .status = 200,
            .headers = "Content-Type: application/json\nCache-Control: max-age=604800",
            .body = fiy::Body(tzdb_json)
        };
        req.respond(cb, tzdb_json_resp);
        return;
    }

    // Everything here requires a login
    if (req.user == nullptr) {
        static const fiy::Response no_auth_resp{
            303,
            "Location: " + g_host_info.host_base_uri() + "/portal/login",
            fiy::Body()
        };

        req.respond(cb, no_auth_resp);
        return;
    }

    if (path.starts_with("/main.css")) {
        req.respond(cb, 200,
            "Content-Type: text/css\nCache-Control: max-age=604800",
            Pages::main_css()
        );
        return;
    }
    if (path.starts_with("/main.js")) {
        req.respond(cb, 200,
            "Content-Type: text/javascript\nCache-Control: max-age=604800",
            Pages::main_js()
        );
        return;
    }

    // Fontawesome fonts
    if (path == "/fa/fa.css") {
        static constexpr char file_path[] = "font-awesome.css";
        req.respond(cb, 200,
            "Content-Type: text/css\nCache-Control: max-age=604800",
            Pages::file_body<file_path>()
        );
        return;
    }
    if (path.starts_with("/fonts/fontawesome-webfont.")) {
        path.remove_prefix(27);
        if (path.starts_with("eot")) {
            static constexpr char file_path[] = "fontawesome-webfont.eot";
            req.respond(cb, 200,
                "Content-Type: application/vnd.ms-fontobject\nCache-Control: max-age=604800",
                Pages::file_body<file_path>()
            );
            return;
        }
        if (path.starts_with("woff2")) {
            static constexpr char file_path[] = "fontawesome-webfont.woff2";
            req.respond(cb, 200,
                "Content-Type: font/woff2\nCache-Control: max-age=604800",
                Pages::file_body<file_path>()
            );
            return;
        }
        if (path.starts_with("woff")) {
            static constexpr char file_path[] = "fontawesome-webfont.woff";
            req.respond(cb, 200,
                "Content-Type: font/woff\nCache-Control: max-age=604800",
                Pages::file_body<file_path>()
            );
            return;
        }
        if (path.starts_with("ttf")) {
            static constexpr char file_path[] = "fontawesome-webfont.ttf";
            req.respond(cb, 200,
                "Content-Type: font/ttf\nCache-Control: max-age=604800",
                Pages::file_body<file_path>()
            );
            return;
        }
        if (path.starts_with("svg")) {
            static constexpr char file_path[] = "fontawesome-webfont.svg";
            req.respond(cb, 200,
                "Cache-Control: max-age=604800",
                Pages::file_body<file_path>()
            );
            return;
        }
    }

    // Landing page
    if (path == "/" || (path.size() > 1 && path[1] == '?')) {
        req.respond(cb, 200, "Content-Type: text/html", Pages::index_html());
        return;
    }

    // Download all contacts
    if (path == "/all") {
        if (req.domain != nullptr)
            return;
        req.respond(cb, 200,
            "Content-Type: text/vcard",
            fiy::Body(DB::get_user_rolodex(req.user))
        );
        return;
    }

    // TODO accept multiple cards in a single file
    if (path == "/save") {
        VC card;
        card.owner = req.user;
        card.parse(std::string(request->body, request->body_len));
        switch (DB::save_contact(card)) {
            case DB::Success:
                req.respond(cb, 200, "Content-Type: text/vcard", fiy::Body(card.to_vcard()));
                return;
            case DB::Error:
                req.respond(cb, 500, "Server Error");
                return;
            case DB::Unauthorized:
                req.respond(cb, 401, "Unauthorized");
                return;
        }
    }

    // Get card by id
    if (path.starts_with("/id/")) {
        path.remove_prefix(4);
        VC card;
        try {
            card.id = std::stoll(std::string(path));
        } catch (...) {
            req.respond(cb, 400, "Invalid contact ID");
            return;
        }
        card.owner = req.user;

        if (DB::get_contact(card))
            req.respond(cb, 200, "Content-Type: text/vcard", fiy::Body(card.to_vcard()));
        else
            req.respond(cb, 404, "No card with given id");
        return;
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

static void delete_user(const char* username) {
    // Invalid input
    if (username == nullptr || username[0] == '\0')
        return;

    // Not a local user -- not our problem
    if (strchr(username, '@') != nullptr)
        return;

    // Delete the user
    DB::delete_user(username);
}

extern "C" fiy::ModInfo* start(const fiy::fiy_host_info_t* host_info) {
    static fiy::ModInfo mod_info = {
        .on_request = handle_request,
        .delete_user = nullptr,
        .id="fiy.contacts",
        .version = "0.0"
    };
    g_host_info = *host_info;

    return &mod_info;
}
