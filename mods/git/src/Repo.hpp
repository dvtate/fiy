//
// Created by tate on 12/5/25.
//

#pragma once

#include <utility>
#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include <optional>

#include "../../../modlib/fediymod.hpp"

struct BasicRepo {
    enum class Access {
        Error = -1,
        Blocked = 0,
        Read = 1,
        Write = 2,
        Admin = 3,
        Owner = 4,
    };

    /// Instance of repo owner
    std::string instance;

    /// Username of the repo owner
    std::string owner;

    /// Repo name
    std::string name;

    /// Get canonical path
    [[nodiscard]] std::string path() const {
        std::string ret = this->owner;
        if (!this->instance.empty())
            ret += '@' + this->instance;
        ret += '/' + this->name;
        return ret;
    }

    bool from_path(std::string url) {
        // shortest url: a/b
        if (url.size() < 3)
            return false;

        // Remove leading ////'s
        if (url[0] == '/')
            url = url.substr(url.find_first_not_of('/'));
        if (url.size() < 3)
            return false;

        // Remove leading @ to escape special usernames
        if (url[0] == '@')
            url = url.substr(1);
        if (url.size() < 3)
            return false;

        // Invalid
        auto slash = url.find('/');
        if (slash == std::string::npos)
            return false;

        // Get Instance
        const auto at = url.find('@');
        if (at == std::string::npos) {
            this->instance = "";
            this->owner = url.substr(0, slash);
        } else {
            // Invalid
            if (at > slash)
                return false;

            this->owner = url.substr(0, at);

            this->instance = url.substr(at, slash - at);
            if (instance == fiy::Host::info.domain)
                this->instance = "";
        }

        // repo name shouldn't start with slash
        slash++;
        if (slash >= url.size())
            return false;

        // Ignore subpath or query strings
        auto end = url.find_first_of("/?", slash);
        if (end == std::string::npos) {
            end = url.size() - 1;
        }

        // Ignore .git
        if (url.compare(end - 5, 4, ".git"))
            end -= 4;

        // Get repo name
        this->name = url.substr(slash, end - slash);

        std::cout <<"BasicRepo::from_path: " <<this->owner << " @ " << this->instance << " / " <<this->name << "\n";
        return !this->name.empty();
    }

    bool is_local() {
        return this->instance.empty();
    }

    bool can_access(
        const Access min_level,
        const char* user,
        const char* domain = nullptr
    );

};

struct RepoInfo : public BasicRepo {

    /// Short description for the repo
    std::string description{};

    /// Default read access (ie - view, clone, etc.)
    fiy::Locality visibility{};

    /// Is this repo a fork of another repo?
    std::string fork_of{};

    /// Local db id if available
    int id = -1;

    /// When was this repo created?
    time_t create_ts;

    /// Get info, from DB/remote instance
    static bool fetch(
        std::string repo_url_path,
        std::function<void(RepoInfo*)> cb,
        const char* user,
        const char* domain
    );

    std::string to_json();
    bool from_json(const std::string& json);

    /// Create a new repo
    const char*  create();
};
