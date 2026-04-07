//
// Created by tate on 2/23/26.
//

#pragma once

#include <mutex>
#include <vector>
#include <git2.h>
#include <utility>
#include <boost/unordered/unordered_flat_map.hpp>

#include "BasicRepo.hpp"

struct RepoPageData;
struct RepoFileBrowserPageData;

/**
 * C++ wrapper for libgit2 operations on git repositories
 *
 * @remark for now this is only for local repos
 * @remark this class should only ever be instantiated by LocalRepo
 *  as duplicate instances for the same repo could be really bad
 */
class GitRepo {

protected:
    /// Git repository
    git_repository* m_repo{nullptr};

    /// Git revision walker
    /// @remark According to the docs, this is expensive to allocate and should be reused
    git_revwalk* m_walker{nullptr};

    /// Thread lock
    std::mutex m_mtx; // TODO replace with RWMutex

public:

    struct User {
        std::string name;
        std::string email;
        std::string fiy_user;

        std::string profile_link() const;
    };

    struct Commit {
        char id[GIT_OID_HEXSZ + 1]{""};
        std::string message;
        time_t ts{0};
        User author;
        User committer;

        explicit Commit(git_commit* commit);
        Commit(git_commit* commit, git_mailmap* mailmap);
        Commit() = default;

        [[nodiscard]] bool valid() const {
            return ts != 0;
        }
    };

    // Git tree entry
    struct Entry {
        enum {
            COMMIT,
            DIRECTORY,
            FILE
        } type;
        std::string path;
        Commit last_commit;
    };

    GitRepo() = default;
    explicit GitRepo(const char* path);

    /**
     * Take ownership of a git repository
     * @param repo git repository object to take ownership of
     */
    explicit GitRepo(git_repository* repo): m_repo(repo) {}

    explicit GitRepo(const BasicRepo& repo);

    GitRepo(const GitRepo& other) = delete;
    GitRepo(GitRepo&& other)  noexcept {
        other.m_mtx.lock();
        m_repo = std::exchange( other.m_repo, nullptr);
        other.m_mtx.unlock();
    }

    ~GitRepo();

    bool open(const BasicRepo& repo);
    int create(const BasicRepo& repo, const char* description = nullptr);

    [[nodiscard]] git_repository* repo() const { return m_repo; }
    [[nodiscard]] bool is_open() const { return m_repo != nullptr; }
    // [[nodiscard]] bool is_local() const { return m_local; }
    std::vector<std::string> tags();
    ssize_t tags_count();
    std::string default_branch();
    std::vector<std::string> branches();
    ssize_t branches_count();
    ssize_t commits_count(const std::string& branch);
    Commit last_commit(const std::string& branch);

    int entries(std::vector<Entry>& ret, const std::string& branch, const std::string& path = "");

protected:
    [[nodiscard]] git_revwalk* walker();
    int branch_tip(const std::string& branch, git_oid& oid);

    ssize_t commits_count(const git_oid* start_oid);
    Commit last_commit(const git_oid* start_oid);
    int entries(std::vector<Entry>& ret, const git_oid* target, const std::string& path = "");
    Commit last_commit(const git_oid* start_oid, const std::string& path, git_mailmap* mailmap);

    bool get_repo_page_data(const std::string& branch, RepoPageData& data);

private:
    static bool ok(int status, const std::string& message= "");
};