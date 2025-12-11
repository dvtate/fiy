//
// Created by tate on 12/8/25.
//

#pragma once

#include <vector>
#include <string>
#include <string_view>

#include "../../../modlib/fediymod.hpp"

extern fiy::HostInfo g_host_info;

struct Repo {
    /// Username of the repo owner
    std::string owner;

    /// Repo name
    std::string name;

    /// Short description for the repo
    std::string description;

    /// Default read access (ie - view, clone, etc.)
    fiy::Locality visibility;

    std::string fork_of{};

    [[nodiscard]] std::string get_path() const {
        return this->owner + '/' + this->name;
    }

    static void get_repo_data(
        std::string_view repo_path,
        void (*cb)(Repo*)
    );

    // These are for storing in stl containers... ugly solution
    struct Hash {
        std::size_t operator()(const Repo& token) const {
            return std::hash<std::string>{}(token.get_path());
        }
        std::size_t operator()(const Repo* token) const {
            return std::hash<std::string>{}(token->get_path());
        }
    };
    struct TokenEquals {
        bool operator()(const Repo& a, const Repo& b) const {
            return a.get_path() == b.get_path();
        }
        bool operator()(const Repo* a, const Repo* b) const {
            return a->get_path() == b->get_path();
        }
    };
    struct Compare {
        bool operator()(const Repo& a, const Repo& b) const {
            return a.get_path() < b.get_path();
        }
        bool operator()(const Repo* a, const Repo* b) const {
            return a->get_path() < b->get_path();
        }
    };
};

struct LocalRepo : Repo {
    size_t m_id;
};

struct RemoteRepo: Repo {

};

struct RepoAccess {

};

struct Repos {
};