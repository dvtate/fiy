//
// Created by tate on 12/5/25.
//

#include "LocalRepo.hpp"

#include <git2.h>
#include <nlohmann/json.hpp>

#include "git_http_backend.hpp"
#include "../DB.hpp"
#include "../Routes/RepoPageData.hpp"

using DB::operator ""_sql;

LocalRepo::LocalRepo(const BasicRepo& repo):
    BasicRepo(repo), GitRepo(repo)
{
    thread_local auto q = "SELECT description, visibility, createTs, id FROM Repos"
        "    WHERE userName=? AND repoName=?"_sql;
    q.bindNoCopy(1, repo.owner);
    q.bindNoCopy(2, repo.name);
    if (!q.executeStep()) {
        q.reset();
        return;
    }
    this->description = q.getColumn(0).getString();
    this->visibility = (fiy::Locality)q.getColumn(1).getInt();
    this->create_ts = q.getColumn(2).getInt64();
    this->id = q.getColumn(3).getInt();
    q.reset();

    thread_local auto q_fork = "SELECT fromRepoPath FROM RepoForks WHERE toRepoPath=?"_sql;
    q_fork.bind(1, this->path());
    if (q_fork.executeStep())
        this->fork_of = q_fork.getColumn(0).getString();
    q_fork.reset();
}

bool LocalRepo::can_access(
    const Access min_level,
    const char* user,
    const char* domain
) const {
    // Return true so that request gets proxied to remote instance
    // if they don't have access, it will be blocked
    // note for local copy case: maybe should add an endpoint to check access
    if (!this->is_local()) {
        return true;
    }

    // If we only need read access, check the
    if (min_level <= Access::Read) {
        switch (this->visibility) {
            case fiy::Locality::PUBLIC:
                return true;
            case fiy::Locality::FEDIVERSE:
                return user != nullptr && domain != nullptr;
            case fiy::Locality::INSTANCE:
                return user != nullptr && domain == nullptr;
            case fiy::Locality::USER:
                return false;
            default:
                fiy::host().log_warning("Invalid visibility value"
                    + std::to_string(static_cast<int>(visibility)));
                return false;
        }
    }

    // Owner has access
    if (domain == nullptr && user != nullptr && this->owner == user)
        return min_level <= Access::Owner;

    // Check if user was granted permission
    return user_can_access_local(min_level, user, domain);
}

const char* LocalRepo::create() {
    std::string repo_path = fiy::host().data_dir;
    repo_path += "/repos/" + owner + "/" + name;

    thread_local auto q_exists = "SELECT 1 FROM Repos WHERE userName=? AND repoName=?"_sql;
    q_exists.bindNoCopy(1, owner);
    q_exists.bindNoCopy(2, name);
    if (q_exists.executeStep()) {
        q_exists.reset();
        return "4XX: Repo with same name already exists";
    }
    q_exists.reset();

    std::string user_repos_dir = fiy::host().data_dir;
    user_repos_dir += "/repos/" + owner;
    if (!std::filesystem::is_directory(user_repos_dir))
        if (!std::filesystem::create_directory(user_repos_dir))
            return "5XX: Failed to create user repos directory";
    if (!std::filesystem::create_directory(repo_path))
        return "5XX: Failed to create repo directory";

    // Set options for a bare repository
    git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    opts.flags = GIT_REPOSITORY_INIT_BARE;
    opts.description = this->description.c_str(); // Optional: set a description

    // Create the bare repository
    git_repository *repo = nullptr;
    const int error = git_repository_init_ext(&repo, repo_path.c_str(), &opts);
    if (error < 0) {
        const git_error *e = giterr_last();
        fiy::host().log_error("Error creating bare repository: " + std::string(e->message));
        return "5XX: Server Error: Failed to create bare git repo.";
    }
    git_repository_free(repo); // Free the repository object

    // Add to database
    thread_local auto q = "INSERT INTO Repos (userName, repoName, description, visibility, createTs)"
        " VALUES (?, ?, ?, ?, ?)"_sql;
    q.bindNoCopy(1, this->owner);
    q.bindNoCopy(2, this->name);
    q.bindNoCopy(3, this->description);
    q.bind(4, (int)this->visibility);
    q.bind(5, this->create_ts = fiy::host().now());
    const bool ret = q.exec() > 0;
    q.reset();
    if (ret)
        return nullptr;
    return "5XX: Database Error: Failed to create database entry";
}

ssize_t LocalRepo::forks_count() const {
    thread_local auto q = "SELECT COUNT(*) FROM RepoForks WHERE fromRepoPath = ?"_sql;

    std::string cannonical_path = this->owner;
    cannonical_path += '@';
    cannonical_path += this->instance.empty() ? fiy::host().domain : this->instance;
    cannonical_path += '/';
    cannonical_path += this->name;

    q.bindNoCopy(1, cannonical_path);
    if (!q.executeStep()) {
        fiy::log_error("Could not select COUNT of repo forks");
        q.reset();
        return 0;
    }
    const ssize_t ret = q.getColumn(0).getInt64();
    q.reset();
    return ret;
}

ssize_t LocalRepo::likes_count() {
    // TODO no database table yet
    return -1;
}

ssize_t LocalRepo::tickets_count() {
    thread_local auto q = "SELECT COUNT(*) FROM RepoTickets WHERE repoId = ?"_sql;

    q.bind(1, this->id);
    if (!q.executeStep()) {
        fiy::log_error("Could not select COUNT of repo tickets");
        q.reset();
        return 0;
    }
    const ssize_t ret = q.getColumn(0).getInt64();
    q.reset();
    return ret;
}

bool LocalRepo::get_repo_page_data(const std::string& branch, RepoPageData& data) {
    // Get data from git
    GitRepo::get_repo_page_data(branch, data);

    data.visibility = this->visibility;
    data.description = this->description;
    data.fork_of = this->fork_of;
    data.owner = this->owner;
    data.name = this->name;
    data.instance = this->instance;
    data.likes_count = this->likes_count();
    data.forks_count = this->forks_count();
    data.tickets_count = this->tickets_count();
    return true;
}
