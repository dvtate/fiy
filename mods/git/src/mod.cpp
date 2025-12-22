//
// Created by tate on 12/2/25.
//

#include <git2.h>
#include <string_view>

#include "../../../modlib/fediymod.hpp"

#include "../../../util/WebUtils.hpp"

#include "git_http_backend.hpp"
#include "Pages.hpp"
#include "Repo.hpp"

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

void repo_create_get(const fiy::Request& req, const fiy::Callback cb) {
    if (req.user == nullptr) {
        unauthenticated(req, cb);
        return;
    }
    if (req.domain != nullptr) {
        req.respond(cb, 401);
        return;
    }

    const std::string page = Pages::repo_create_page(req.user, {});
    req.respond(cb, 200,
        "Content-Type: text/html; charset=utf-8",
        fiy::Body(page)
    );
}

void repo_create_post(const fiy::Request& req, const fiy::Callback cb) {
    if (req.user == nullptr) {
        unauthenticated(req, cb);
        return;
    }
    if (req.domain != nullptr) {
        req.respond(cb, 401);
        return;
    }

    std::deque<std::pair<std::string, std::string>> form;
    const bool ok = WebUtils::parse_form_url_encoded(
        std::string_view(req.body, req.body_len),
        form);
    if (!ok) {
        req.respond(cb, 400, "", "Invalid Form Body");
        fiy::Host::info.log_info("Repo create: " + req.user_str() + " submitted invalid form body");
        return;
    }

    RepoInfo repo;
    for (const auto& [k, v]: form) {
        if (k == "repo-owner") {
            repo.owner = v;
        } else if (k == "repo-name") {
            repo.name = v;
        } else if (k == "repo-description") {
            repo.description = v;
        } else if (k == "repo-visibility") {
            repo.visibility = (fiy::Locality) atoi(v.c_str()); // zero is failsafe
        } else {
            fiy::Host::info.log_info("Repo create: " + req.user_str() + " gave invalid field: " + k);
        }
    }

    if (repo.owner != req.user) {
        // TODO allow users to create org repos
        req.respond(cb, 401);
        return;
    }

    const char* err = repo.create();
    if (err != nullptr) {
        std::string msg = "<h1>That didn't work!</h1><br/>";
        msg += err;
        req.respond(cb, err[0] == '4' ? 400 : 500, "", fiy::Body(msg));
        return;
    }

    std::string body;
    body += "<meta http-equiv=\"refresh\" content=\"0; url=";
    body += fiy::Host::info.base_uri;
    body += '/' + repo.path();
    body += "\" />";
    req.respond(cb, 200, "Content-Type: text/html; charset=utf-8", body);
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

    if (path.starts_with("/repo")) {
        if (req.locality(fiy::Host::info.domain) > fiy::Locality::INSTANCE) {
            unauthenticated(req, cb);
            return;
        }
        path.remove_prefix(5);
        std::cout <<"Req.user: " << req.user_str("xxx") <<std::endl;
        if (path.starts_with("/new")) {
            if (req.method == fiy::Request::Method::GET) {
                repo_create_get(req, cb);
                return;
            }
            if (req.method == fiy::Request::Method::POST) {
                repo_create_post(req, cb);
                return;
            }
        }
        not_found_404(req, cb);
        return;
    }

    if (path.starts_with("/api")) {
        // TODO
        not_found_404(req, cb);
        return;
    }

    // else it's a repo

    BasicRepo repo;
    if (!repo.from_path(req.path)) {
        not_found_404(req, cb);
        return;
    }

    auto access = RepoInfo::Access::Read;
    if (std::string_view(req.path).find("git-receive-pack") != std::string_view::npos)
        access = RepoInfo::Access::Write;
    if (!repo.can_access(access, req.user, req.domain)) {
        std::cerr <<"NO access!\n";
        std::cout <<req.headers <<std::endl;
        req.respond(cb, 401);
        // not_found_404(req, cb);
        return;
    }

    // if (req.find_header("user-agent").starts_with("git/"))
        git_repo_cgi(req, cb);
}

void delete_user(const char* username) {
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
        fiy::Host::info.log_error(msg);
    }
}