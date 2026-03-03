//
// Created by tate on 12/2/25.
//

#include <git2.h>
#include <string_view>

#include "../../../modlib/fediymod.hpp"

#include "../../../util/WebUtils.hpp"

#include "Repos/git_http_backend.hpp"
#include "Routes/Pages.hpp"
#include "Repos/LocalRepo.hpp"
#include "Routes/RepoRouter.hpp"


fiy::Host fiy::Host::info;

/// User is unauthenticated, send them to login page
void unauthenticated(const fiy::Request& req, const fiy::Callback cb) {
    static const fiy::Response no_auth_resp{
        303,
        "Location: " + fiy::Host::info.host_base_uri() + "/portal/login",
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

    std::cout <<"GIT path: " << path << std::endl;
    if (path == "/") {
        static constexpr char file_path[] = "/landing.html";
        req.respond(cb, 200,
            "Content-Type: text/html; charset=utf-8\nCache-Control: max-age=604800",
            Pages::file_body<file_path>()
        );
        return;
    }

    // Handle repo routes
    if (repo_request_router(path, cb, req))
        return;
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
extern "C" fiy::ModInfo* start(const fiy_host_info_t* host_info) {
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
    fiy::Host::set(*host_info);
    return &mod_info;
}

/// Export: Stop module
extern "C" void stop() {
    // Shutdown libgit2
    if (git_libgit2_shutdown() < 0) {
        const git_error *e = giterr_last();
        std::string msg = "Failed to shutdown libgit2: ";
        msg += e->message;
        fiy::log_error(msg);
    }
}