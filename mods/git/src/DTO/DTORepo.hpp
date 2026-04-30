//
// Created by tate on 4/30/26.
//

#pragma once

#include <vector>

#include "../Repos/LocalRepo.hpp"
#include "../Repos/GitRepo.hpp"
#include "DataTransferObject.hpp"

// TODO should probably nlohmann json's to_json/from_json functionality
//      but it's designed to use exceptions and it's not pretty so idk.
//      I'll just half-ass it for now.

struct DTORepo : DataTransferObject {

    /// List of files/dirs/objects
    struct DTORepoEntries : DataTransferObject {
        std::vector<GitRepo::Entry> entries;

        bool from_json(const nlohmann::json& json) final;
        nlohmann::json to_json() final;
    };

    /// Information that could vary by branch/subpath/ref/etc.
    struct DTORepoTreeData : DataTransferObject {
        GitRepo::Commit last_commit;
        std::string active_branch;
        std::string path;

        bool from_json(const nlohmann::json& json) final;
        nlohmann::json to_json() final;
    };

    /// Repo stats
    struct DTORepoStats {
        ssize_t commits_count{-1};
        ssize_t tags_count{-1};
        ssize_t branches_count{-1};
        ssize_t likes_count{-1};
        ssize_t tickets_count{-1};
        ssize_t forks_count{-1};
    };

    /// Basic repo info
    struct DTORepoInfo {
        /// Instance of repo owner
        std::string instance;

        /// Username of the repo owner
        std::string owner;

        /// Repo name
        std::string name;

        /// User provided description for the repo
        std::string description;

        /// Is this repo a fork of another repo?
        std::string fork_of{};

        /// When was this repo created?
        time_t create_ts{0};

        /// Repo default visibility
        fiy::Locality visibility{fiy::Locality::USER};

        std::string default_branch;
    };

    DTORepoEntries entries;
    DTORepoTreeData tree;
    DTORepoStats stats;
    DTORepoInfo info;

    bool from_json(const nlohmann::json& json) final;
    nlohmann::json to_json() final;
};
