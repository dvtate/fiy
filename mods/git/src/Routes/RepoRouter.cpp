//
// Created by tate on 2/27/26.
//

#include "RepoRouter.hpp"

#include <deque>
#include "../../../util/WebUtils.hpp"
#include "../Repos/BasicRepo.hpp"
#include "Pages.hpp"
#include "../Repos/Repos.hpp"

/// User is unauthenticated, send them to login page
static void unauthenticated(const fiy::Request& req, const fiy::Callback cb) {
    if (req.user == nullptr) {
        // Anon/unauthenticated local user
        static const fiy::Response no_auth_resp{
            303,
            "Location: " + fiy::host().host_base_uri() + "/portal/login",
            fiy::Body()
        };
        req.respond(cb, no_auth_resp);
    } else {
        // Unauthenticated remote API user
        req.respond(cb, 401);
    }
}

/// Show the new repo page
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

/// Handle repo create form action
void repo_create_post(const fiy::Request& req, const fiy::Callback cb) {
    if (req.user == nullptr) {
        unauthenticated(req, cb);
        return;
    }
    if (req.domain != nullptr) {
        // TODO this should eventually be allowed for orgs?
        req.respond(cb, 401);
        return;
    }

    // Parse form body
    std::deque<std::pair<std::string, std::string>> form;
    const bool ok = WebUtils::parse_form_url_encoded(
        std::string_view(req.body, req.body_len),
        form);
    if (!ok) {
        req.respond(cb, 400, "", "Invalid Form Body");
        fiy::log_info("Repo create: " + req.user_str() + " submitted invalid form body");
        return;
    }

    /// Construct repo from form data
    LocalRepo repo;
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
            fiy::log_info("Repo create: " + req.user_str() + " gave invalid field: " + k);
        }
    }

    // TODO allow users to create org repos
    if (repo.owner != req.user) {
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

    // Send the user to the newly created repo
    std::string body;
    body += "<meta http-equiv=\"refresh\" content=\"0; url=";
    body += fiy::host().base_uri;
    body += '/' + repo.path();
    body += "\" />";
    req.respond(cb, 200, "Content-Type: text/html; charset=utf-8", body);
}



bool repo_request_router(
    std::string_view path,
    fiy::Callback cb,
    fiy::Request& req
) {
    if (path.empty())
        return false;

    // Repo create
    if (path.starts_with("/repo")) {
        if (req.locality(fiy::host().domain) > fiy::Locality::INSTANCE) {
            unauthenticated(req, cb);
            return true;
        }
        path.remove_prefix(5);
        std::cout <<"Req.user: " << req.user_str("xxx") <<std::endl;
        if (path.starts_with("/new")) {
            if (req.method == fiy::Request::Method::GET) {
                repo_create_get(req, cb);
                return true;
            }
            if (req.method == fiy::Request::Method::POST) {
                repo_create_post(req, cb);
                return true;
            }
        }
        return false;
    }

    BasicRepo basic_repo;
    if (!basic_repo.from_path(path))
        return false;

    // Handle remote repo
    if (!basic_repo.is_local()) {
        // TODO properly handle remote request
        // I think this only works for unauthenticated git pull
        req.domain = basic_repo.instance.c_str();
        fiy::host().request_mod(fiy::host().app_id, &req,
            [req, cb](const fiy_response_t* res) {
                req.respond(cb, *res);
            });
        return false;
    }

    // Handle local repo
    LocalRepo& repo = *Repos::cache.local_repo(basic_repo);
    if (!repo.valid())
        return false;

    //
    auto access = LocalRepo::Access::Read;
    auto slash = path.find('/', 1);
    if (slash == std::string_view::npos)
        return false;
    slash = path.find('/', slash + 1);
    if (slash == std::string_view::npos) {
        if (!repo.can_access(access, req.user, req.domain)) {
            unauthenticated(req, cb);
            return true;
        }
        RepoPageData data;
        repo.get_repo_page_data(repo.default_branch(), data);
        const auto body = Pages::repo_page(data);
        req.respond(cb,
            200,
            "Content-Type: text/html",
            fiy::Body(body)
        );
        return true;
    }

    // Else, we don't know the path, send it to the cgi
    if (std::string_view(req.path).find("git-receive-pack") != std::string_view::npos)
        access = LocalRepo::Access::Write;
    if (!repo.can_access(access, req.user, req.domain)) {

        // Need to send special header to git client
        if (req.find_header("User-Agent").starts_with("git/")) {
            req.respond(cb, 401,
                "WWW-Authenticate: Basic charset=\"UTF-8\"");
            return true;
        }
        unauthenticated(req, cb);
        return true;
    }

    // Paths:
    // /user/repo               -- summary page (default branch)
    // /user/repo/tickets       -- issues/pr tab
    // /user/repo/settings      -- settings page (must check permissions)

    // we don't know what to do, pass it on to the cgi
    repo.http_cgi(req, cb);

    return true; // we have to be called last
}
