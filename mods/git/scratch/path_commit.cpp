#include <git2.h>
#include <string>
#include <stdexcept>
#include <cstring>
#include <iostream>

struct FileCommitInfo {
    std::string commit_id;
    std::string message;
    std::string author_name;
    std::string author_email;
    git_time_t  timestamp;
};

// Helper: check libgit2 return codes
static void check_error(int error_code, const char* message) {
    if (error_code < 0) {
        const git_error* err = git_error_last();
        std::string msg = message;
        if (err && err->message) {
            msg += ": ";
            msg += err->message;
        }
        throw std::runtime_error(msg);
    }
}

// Determines if path is directory in HEAD tree
static bool is_directory(git_repository* repo, const std::string& path) {
    git_object* tree_obj = nullptr;
    check_error(
        git_revparse_single(&tree_obj, repo, "HEAD^{tree}"),
        "Failed to get HEAD tree"
    );

    git_tree* tree = (git_tree*)tree_obj;

    git_tree_entry* entry = nullptr;
    int ret = git_tree_entry_bypath(&entry, tree, path.c_str());

    git_tree_free(tree);

    if (ret == GIT_ENOTFOUND)
        return false; // Doesn't exist
    check_error(ret, "Failed to look up path");

    git_object_t type = git_tree_entry_type(entry);
    git_tree_entry_free(entry);

    return type == GIT_OBJECT_TREE;
}

// Main function
FileCommitInfo get_latest_commit_for_path(git_repository* repo,
                                          const std::string& input_path)
{
    git_revwalk* walker = nullptr;
    check_error(git_revwalk_new(&walker, repo), "Failed to create revwalk");

    git_revwalk_sorting(walker, GIT_SORT_TIME | GIT_SORT_TOPOLOGICAL);

    // Start from HEAD
    check_error(git_revwalk_push_head(walker), "Failed to push HEAD");

    std::string path = input_path;
    bool directory = is_directory(repo, path);

    if (directory) {
        if (path.back() != '/')
            path += '/';
    }

    git_oid oid;
    while (git_revwalk_next(&oid, walker) == 0) {

        git_commit* commit = nullptr;
        check_error(git_commit_lookup(&commit, repo, &oid),
                    "Failed to lookup commit");

        git_tree* tree = nullptr;
        check_error(git_commit_tree(&tree, commit),
                    "Failed to get commit tree");

        git_tree* parent_tree = nullptr;

        if (git_commit_parentcount(commit) > 0) {
            git_commit* parent = nullptr;
            check_error(git_commit_parent(&parent, commit, 0),
                        "Failed to get parent commit");

            check_error(git_commit_tree(&parent_tree, parent),
                        "Failed to get parent tree");

            git_commit_free(parent);
        }

        git_diff* diff = nullptr;
        check_error(
            git_diff_tree_to_tree(&diff, repo, parent_tree, tree, nullptr),
            "Failed to diff trees"
        );

        size_t deltas = git_diff_num_deltas(diff);

        for (size_t i = 0; i < deltas; ++i) {
            const git_diff_delta* delta = git_diff_get_delta(diff, i);

            const char* old_path = delta->old_file.path;
            const char* new_path = delta->new_file.path;

            auto matches = [&](const char* p) -> bool {
                if (!p) return false;
                if (directory) {
                    return std::strncmp(p, path.c_str(), path.size()) == 0;
                } else {
                    return path == p;
                }
            };

            if (matches(old_path) || matches(new_path)) {

                const git_signature* author = git_commit_author(commit);

                FileCommitInfo info;
                info.commit_id = git_oid_tostr_s(&oid);
                info.message = git_commit_message(commit);
                info.author_name = author ? author->name : "";
                info.author_email = author ? author->email : "";
                info.timestamp = author ? author->when.time : 0;

                std::cout <<"Commit:"
                    <<"\n\tid:"<<git_oid_tostr_s(&oid)
                    <<"\n\tmessage:"<<git_commit_message(commit)
                    <<"\n\tsummary:"<<git_commit_summary(commit)
                    <<"\n\tbody:"<<git_commit_body(commit)
                    <<std::endl;

                git_diff_free(diff);
                git_tree_free(tree);
                if (parent_tree) git_tree_free(parent_tree);
                git_commit_free(commit);
                git_revwalk_free(walker);

                return info;
            }
        }

        git_diff_free(diff);
        git_tree_free(tree);
        if (parent_tree) git_tree_free(parent_tree);
        git_commit_free(commit);
    }

    git_revwalk_free(walker);
    throw std::runtime_error("No commit found impacting the given path");
}


int main() {
    git_libgit2_init();
    const char* repo_path = "/tmp/corki.git";
    const char* subdir = "web";

    git_repository* repo = nullptr;
    if (git_repository_open(&repo, repo_path) < 0) {

        const git_error* e = git_error_last();
        std::string log_msg = "Error";
        log_msg += ": ";
        log_msg += e != nullptr ? e->message : "Unknown error";
        std::cerr << log_msg << std::endl;
        std::cerr << "Failed to open repo " << repo_path << std::endl;
        return 1;
    }


    auto _ = get_latest_commit_for_path(repo, subdir);
    std::cout << _.author_name <<" " <<_.author_email <<std::endl;
}