//
// Created by tate on 12/8/25.
//

#include "git.hpp"

#include "../../../modlib/fediymod.hpp"

#include <git2.h>

#include <string>

extern fiy::HostInfo g_host_info;

namespace git {
    int create_empty_repo(const std::string& user, const std::string& repo_name) {
        std::string repo_path = g_host_info.data_dir;
        repo_path += "/repos/" + user + "/" + repo_name;

        // Set options for a bare repository
        git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
        opts.flags = GIT_REPOSITORY_INIT_BARE;
        opts.description = "My new bare repository"; // Optional: set a description

        // Create the bare repository
        git_repository *repo = nullptr;
        const int error = git_repository_init_ext(&repo, repo_path.c_str(), &opts);

        if (error < 0) {
            const git_error *e = giterr_last();
            fprintf(stderr, "Error creating bare repository: %s\n", e->message);
        } else {
            printf("Bare repository created successfully at: %s\n", repo_path.c_str());
            git_repository_free(repo); // Free the repository object
        }

        return error;
    }

}