//
// Created by tate on 3/2/26.
//

#include "ApiRouter.hpp"

#include <charconv>

#include <nlohmann/json.hpp>

#include "../Repos/BasicRepo.hpp"
#include "../Repos/RepoSearch.hpp"
#include "../DB.hpp"

using DB::operator ""_sql;

void send_to_peer(fiy::Callback cb, fiy::Request& req) {
    fiy::host().request_mod(fiy::host().app_id, &req, [cb, req](const fiy::Response* res) {
        if (res == nullptr)
            req.respond(cb, 500,
                "Content-Type: application/json",
                R"({"error":"Peer request failed"})");
        else
            req.respond(cb, *res);
    });
}

bool repo_api(
    std::string_view path,
    fiy::Callback cb,
    fiy::Request& req
) {

    if (path.starts_with("/info")) {
        BasicRepo repo;
        repo.from_path(path);
        if (!repo.is_local()) {
            req.domain = repo.instance.c_str();
            send_to_peer(cb, req);
            return true;
        }
        req.respond(cb, 500,
            "content-type: text/plain",
            fiy::Body("not implemented"));
        return true;
    } else if (path.starts_with("/search")) {
        path.remove_prefix(7);
        RepoSearch search;

        std::cout <<"Search: " <<path << std::endl;

        if (path.starts_with("/"))
            path.remove_prefix(1);

        while (!path.empty()) {
            if (path[0] == '&' || path[0] == '?') {
                path.remove_prefix(1);
            }

            // Parse query param key + value
            auto eq = path.find_first_of("=&");
            std::string_view key;
            std::string_view value;
            if (eq == std::string_view::npos) {
                key = path;
                value = "";
                path = "";
            } else if (path[eq] == '=') {
                key = path.substr(0, eq);
                auto end = path.find('&', eq);
                value = path.substr(eq + 1, end == std::string_view::npos
                    ? end : (end - eq));
                if (end == std::string_view::npos)
                    path = "";
                else
                    path.remove_prefix(end);
            } else if (path[eq] == '&') {
                key = path.substr(0, eq);
                value = "";
                path.remove_prefix(eq);
            }

            std::cout <<"KV: " <<key <<" = '" <<value <<"'\n";

            // Populate search object
            if (key == "asc") {
                search.order_desc = false;
            } else if (key == "desc") {
                search.order_desc = true;
            } else if (key == "forks") {
                search.include_forks(value == "no"
                    ? RepoSearch::BooleanFilter::No
                    : RepoSearch::BooleanFilter::Yes);
            } else if (key == "visibility") {
                auto n = value[0] - '0';
                if (n > 3 || n < 0)
                    n = 3; // invalid -> public
                search.set_visibility_filter(static_cast<fiy::Locality>(n));
            } else if (key == "owner") {
                search.owner = value;
            } else if (key == "sort") {
                if (value == "age")
                    search.sort = RepoSearch::Sort::Age;
                else if (value == "likes")
                    search.sort = RepoSearch::Sort::Likes;
                else if (value == "name")
                    search.sort = RepoSearch::Sort::Name;
                else if (value == "edit")
                    search.sort = RepoSearch::Sort::EditDate;
            } else if (key == "name") {
                search.set_name_like(std::string(value));
            } else if (key == "description") {
                search.set_description_like(std::string(value));
            } else if (key == "q" || key == "query") {
                search.query = value;
            } else if (key == "min_likes") {
                std::from_chars(value.data(), value.data() + value.size(),
                    search.min_likes);
            } else if (key == "n" || key == "limit") {
                std::from_chars(value.data(), value.data() + value.size(),
                    search.limit);
            } else if (key == "page") {
                std::from_chars(value.data(), value.data() + value.size(),
                    search.page);
            }
        }

        // TODO maybe they want more info (description, likes, etc.)
        try {
            auto repos = search.search(req.user, req.domain);
            auto ret = nlohmann::json::array();
            for (auto& r : repos)
                ret.emplace_back(r.path());
            std::string body = ret.dump();
            req.respond(cb, 200,
                "Content-type: application/json",
                fiy::Body(body));
            return true;
        } catch (SQLite::Exception& e) {
            req.respond(cb, 500,
                "Content-type: application/json",
                fiy::Body("[]"));
            return true;
        }
    }
    return false;
}

bool user_api(
    std::string_view path,
    const fiy::Callback cb,
    fiy::Request& req
) {
    // /api/user/:user/...endpoint

    // Parse user from path
    auto user_end = path.find_first_of("/?#");
    if (user_end == std::string_view::npos) {
        req.respond(cb, 404, "Content-type: text/plain", "/api/user/:user endpoint expected user string");
        return true;
    }
    auto user_str = path.substr(0, user_end);
    auto dom_sep = user_str.find('@');
    if (dom_sep != std::string_view::npos) {

        // Remote user - proxy request
        auto domain = user_str.substr(dom_sep + 1);
        if (domain != fiy::host().domain) {
            // remote request
            auto remote_domain = std::string(domain);
            req.domain = remote_domain.c_str();
            fiy::host().request_mod(fiy::host().app_id, &req,
                [&](const fiy::Response* res) {
                    if (res == nullptr) {
                        req.respond(cb, 500,
                            "Content-type: text/plain",
                            fiy::Body("Failed to connect to peer"));
                    } else {
                        req.respond(cb, *res);
                    }
                });
            return true;
        }

        // Local user - remove explicit domain specifier
        user_str = user_str.substr(0, dom_sep);
    }


    if (path.starts_with("/likes_count")) {
        thread_local auto q = "SELECT COUNT(*) FROM RepoLikes WHERE user=?"_sql;
        if (!q.executeStep()) {
            fiy::host().log_error("User API: DB error: unable to SELECT COUNT(*) FROM RepoLikes");
            req.respond(cb, 500, "Content-type: text/plain", fiy::Body("Database Error"));
        } else {
            const auto count = q.getColumn(0).getInt64();
            const auto body = std::to_string(count);
            req.respond(cb, 200, "Content-type: application/json", body);
        }
        q.reset();
        return true;
    }

    if (path.starts_with("/likes")) {
        path.remove_prefix(6);

        // Parse query params
        auto params = req.query_params();

        uint16_t limit = 200;
        uint16_t page = 0;

        auto it = params.find("limit");
        if (it == params.end())
            it = params.find("n");
        if (it != params.end()) {
            int user_limit;
            auto [ptr, ec] = std::from_chars(
                it->second.data(),
                it->second.data() + it->second.size(),
                user_limit);
            constexpr int MAX_LIMIT = 10'000;
            if (ec != std::errc{} || user_limit < 0 || user_limit > MAX_LIMIT) {
                req.respond(cb, 400, "Content-Type: text/plain", "Invalid limit parameter");
                return true;
            }
            limit = static_cast<uint16_t>(user_limit);
        }

        it = params.find("page");
        if (it != params.end()) {
            int user_page;
            auto [ptr, ec] = std::from_chars(
                it->second.data(),
                it->second.data() + it->second.size(),
                user_page);
            if (ec != std::errc{} || user_page < 0) {
                req.respond(cb, 400, "Content-Type: text/plain", "Invalid page parameter");
                return true;
            }
        }

        // Get liked repos from DB
        thread_local auto q = "SELECT repoPath FROM RepoLikes WHERE user=? LIMIT ? OFFSET ?"_sql;
        q.bind(1, std::string(user_str));
        q.bind(2, limit);
        q.bind(3, (uint32_t)page * (uint32_t)limit);
        std::vector<std::string> repos;
        repos.reserve(std::min((size_t)limit, 200ul));
        while (q.executeStep())
            repos.emplace_back(q.getColumn(0).getString());

        if (repos.empty()) {
            req.respond(cb, 200, "Content-type: application/json", fiy::Body("[]"));
            return true;
        }

        // Calculate JSON string size
        size_t size = 2; // []
        size += repos.size() * 3 - 1; // "", w/o trailing comma
        for (auto& r : repos)
            size += r.size();

        // Generate JSON string
        // NOTE: this is safe because repo names aren't allowed to have '\' or '"' characters
        std::string ret;
        ret.reserve(size);
        ret += "[\"";
        ret += repos[0];
        for (size_t i = 1; i < repos.size(); ++i) {
            ret += "\",\"";
            ret += repos[i];
        }
        ret += "\"]";

        // Send JSON
        req.respond(cb, 200,
            "Content-type: application/json",
            fiy::Body(ret));
        return true;
    }
    return false;
}

bool api_router(
    std::string_view path,
    const fiy::Callback cb,
    fiy::Request& req
) {
    // Remove API prefix
    if (!path.starts_with("/api"))
        return false;
    path.remove_prefix(4);
    if (path.starts_with("/v1"))
        path.remove_prefix(3);

    if (path.empty()) {
        // I added code to prevent users from signing up with this name
        // but leaving this to be safe
        // if (fiy::host().user_info("api", nullptr) == 0)
        //     req.respond(cb, 200,
        //         "Content-type: text/html",
        //         "<h1>Congrats!</h1>"
        //         "<p>Your username is a reserved path!</p>"
        //         "<p>In order to use this mod, please add an @ symbol to your username. For example, \"/@api\".</p>"
        //         "<p>Sorry for any trouble</p>");
        // else
            req.respond(cb, 400, "Not Found");
        return true;
    }

    // Repo API endpoints
    if (path.starts_with("/repo")) {
        path.remove_prefix(5);
        if (!repo_api(path, cb, req))
            req.respond(cb, 404, "", "Not Found");
        return true;
    }

    // User API endpoints
    if (path.starts_with("/user")) {
        path.remove_prefix(5);
        if (!user_api(path, cb, req))
            req.respond(cb, 404, "", "Not Found");
        return true;
    }

    // Invalid endpoint
    req.respond(cb, 404, "", "Not Found");
    return true;
}
