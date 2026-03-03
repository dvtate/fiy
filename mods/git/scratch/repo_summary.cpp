#include <git2.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

static void check_error(int error, const std::string& message) {
    if (error < 0) {
        const git_error* e = git_error_last();
        std::cerr << message << ": "
                  << (e && e->message ? e->message : "Unknown error")
                  << std::endl;
        exit(1);
    }
}

void print_tree(git_repository* repo, git_tree* tree, const std::string& prefix = "") {
    size_t count = git_tree_entrycount(tree);

    for (size_t i = 0; i < count; ++i) {

        const git_tree_entry* entry = git_tree_entry_byindex(tree, i);
        const char* name = git_tree_entry_name(entry);
        git_object_t type = git_tree_entry_type(entry);

        std::cout <<type <<" -- " <<prefix << name;

        if (type == GIT_OBJECT_TREE) {
            std::cout << "/" << std::endl;

            git_tree* subtree = nullptr;
            check_error(
                git_tree_lookup(&subtree, repo, git_tree_entry_id(entry)),
                "Failed to lookup subtree"
            );

            print_tree(repo, subtree, prefix + "  ");
            git_tree_free(subtree);
        } else {
            std::cout << std::endl;
        }
    }
}

void print_recent_commits(git_repository* repo, size_t max_count = 10) {
    git_revwalk* walker = nullptr;
    check_error(git_revwalk_new(&walker, repo), "Failed to create revwalk");

    git_revwalk_sorting(walker, GIT_SORT_TIME | GIT_SORT_TOPOLOGICAL);

    check_error(
        git_revwalk_push_head(walker),
        "Failed to push HEAD"
    );

    std::cout << "\nRecent commits:\n";

    git_oid oid;
    size_t count = 0;

    while (count < max_count && git_revwalk_next(&oid, walker) == 0) {
        git_commit* commit = nullptr;
        check_error(git_commit_lookup(&commit, repo, &oid), "Commit lookup failed");

        const char* message = git_commit_summary(commit);
        const git_signature* author = git_commit_author(commit);

        char oid_str[8];
        git_oid_tostr(oid_str, sizeof(oid_str), &oid);

        std::cout << oid_str << " | "
                  << author->name << " | "
                  << message << std::endl;

        git_commit_free(commit);
        ++count;
    }

    git_revwalk_free(walker);
}

void print_branches(git_repository* repo) {
    git_branch_iterator* it = nullptr;
    check_error(
        git_branch_iterator_new(&it, repo, GIT_BRANCH_LOCAL),
        "Failed to create branch iterator"
    );

    std::cout << "\nBranches:\n";

    git_reference* ref;
    git_branch_t type;

    git_branch_lookup()

    while (git_branch_next(&ref, &type, it) == 0) {
        const char* name = nullptr;
        git_branch_name(&name, ref);

        std::cout << name;

        if (git_branch_is_head(ref)) {
            std::cout << " (current)";
        }

        std::cout << std::endl;

        git_reference_free(ref);
    }

    git_branch_iterator_free(it);
}

void print_tags(git_repository* repo) {
    git_strarray tags;
    check_error(git_tag_list(&tags, repo), "Failed to list tags");

    std::cout << "\nTags:\n";

    for (size_t i = 0; i < tags.count; ++i) {
        std::cout << tags.strings[i] << std::endl;
    }

    git_strarray_dispose(&tags);
}

void print_repo_metadata(git_repository* repo) {
    const char* workdir = git_repository_workdir(repo);
    std::cout << "Repository path: "
              << (workdir ? workdir : "(bare repo)")
              << std::endl;

    git_reference* head = nullptr;
    if (git_repository_head(&head, repo) == 0) {
        const char* branch = git_reference_shorthand(head);
        std::cout << "Default branch: " << branch << std::endl;
        git_reference_free(head);
    }
}

void generate_repo_summary(const std::string& repo_path) {
    git_libgit2_init();

    git_repository* repo = nullptr;
    check_error(
        git_repository_open(&repo, repo_path.c_str()),
        "Failed to open repository"
    );

    std::cout << "=== Repository Summary ===\n";

    print_repo_metadata(repo);

    // HEAD tree
    git_object* head_obj = nullptr;
    check_error(
        git_revparse_single(&head_obj, repo, "HEAD^{tree}"),
        "Failed to get HEAD tree"
    );

    git_tree* tree = (git_tree*)head_obj;

    std::cout << "\nFiles at HEAD:\n";
    print_tree(repo, tree);

    print_recent_commits(repo);
    print_branches(repo);
    print_tags(repo);

    git_tree_free(tree);
    git_repository_free(repo);
    git_libgit2_shutdown();
}


int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr<<"not enough args\n";
    exit(-1);
  }
  generate_repo_summary(argv[1]);
}