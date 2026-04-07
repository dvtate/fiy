//
// Created by tate on 12/2/25.
//

#include <git2.h>
#include <string_view>

#include "../../../modlib/fiymod.hpp"

#include "../../../util/WebUtils.hpp"

#include "Repos/git_http_backend.hpp"
#include "Routes/ApiRouter.hpp"
#include "Routes/Pages.hpp"
#include "Routes/AssetRouter.hpp"
#include "Routes/RepoRouter.hpp"

/// User is unauthenticated, send them to login page
void unauthenticated(const fiy::Request& req, const fiy::Callback cb) {
    static const fiy::Response no_auth_resp{
        303,
        "Location: " + fiy::host().host_base_uri() + "/portal/login",
        fiy::Body()
    };

    req.respond(cb, no_auth_resp);
}

void not_found_404(const fiy::Request& req, const fiy::Callback cb) {
    // TODO
    req.respond(cb, 404, "", "Not Found");
}

void handle_request(struct fiy::fiy_request_t* request, fiy::Callback cb) {
    auto& req = *(fiy::Request*)request;

    std::string_view path = req.path;

    if (path == "/") {
        static constexpr char file_path[] = "/landing.html";
        req.respond(cb, 200,
            "Content-Type: text/html; charset=utf-8\nCache-Control: max-age=604800",
            Pages::file_body<file_path>()
        );
        return;
    }

    if (api_router(path, cb, req)
        || repo_request_router(path, cb, req)
        || static_asset_router(path, cb, req))
        return;

    // No router
    not_found_404(req, cb);
}

void delete_user(const char* username) {
    fiy::log_warning("Git module currently does not handle user deletion");
    // TODO delete their repos, interactions, etc.
    if (strchr(username, '@') != nullptr) {
        // TODO delete RepoAccess entries
        // TODO delete OrgMembers entries
        return;
    }

    // Delete user from database
    // Delete user's repos folder
}

/// Export: Start module
FIY_EXPORT fiy::ModInfo* start(const fiy_host_info_t* host_info) {
    // Initialize libgit2
    if (git_libgit2_init() < 0) {
        const git_error *e = giterr_last();
        std::string message = "Failed to initialize libgit2: ";
        message += e->message;
        host_info->log(
            (int)fiy::Host::Log::FATAL,
            message.c_str());
        return nullptr;
    }

    // TODO verify database and directory structure
    // TODO fix database and directory structure if needed

    // Exchange info
    static fiy::ModInfo mod_info = {
        .on_request = handle_request,
        .delete_user = delete_user,
        .id="git",
        .version = "0.0"
    };
    fiy::host() = *host_info;
    return &mod_info;
}

/// Export: Stop module
FIY_EXPORT void stop() {
    // Shutdown libgit2
    if (git_libgit2_shutdown() < 0) {
        const git_error *e = giterr_last();
        std::string msg = "Failed to shutdown libgit2: ";
        msg += e->message;
        fiy::log_error(msg);
    }
}