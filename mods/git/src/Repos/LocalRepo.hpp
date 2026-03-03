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

#include "../../../../modlib/fediymod.hpp"

#include "BasicRepo.hpp"
#include "GitRepo.hpp"

struct RepoPageData;
struct RepoFileBrowserPageData;

class LocalRepo : public BasicRepo, public GitRepo {
public:
    /// Short description for the repo
    std::string description{};

    /// Default read access (ie - view, clone, etc.)
    fiy::Locality visibility{};

    /// Is this repo a fork of another repo?
    std::string fork_of{};

    /// Local db id if available
    int id = -1;

    /// When was this repo created?
    time_t create_ts{-1};

    [[nodiscard]] bool valid() const {
        return create_ts != -1;
    }

    LocalRepo() = default;
    explicit LocalRepo(const BasicRepo& repo);

    std::string to_json();
    bool from_json(const std::string& json);

    /// Create a new repo
    const char* create();

    bool can_access(
        const Access min_level,
        const char* user,
        const char* domain = nullptr
    ) const;

    bool get_repo_page_data(const std::string& branch, RepoPageData& data);
    bool get_repo_page_data(const std::string& branch, const std::string& path, RepoFileBrowserPageData& data);

};
