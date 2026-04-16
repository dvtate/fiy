#include <git2.h>
#include <iostream>
#include <string>

std::string get_file_contents(git_repository* repo, const std::string& path) {
    git_object* obj = nullptr;
    git_commit* commit = nullptr;
    git_tree* tree = nullptr;
    git_tree_entry* entry = nullptr;
    git_blob* blob = nullptr;

    std::string result;

    // Resolve HEAD to a commit
    if (git_revparse_single(&obj, repo, "HEAD") != 0)
        goto cleanup;

    commit = (git_commit*)obj;

    // Get the tree from the commit
    if (git_commit_tree(&tree, commit) != 0)
        goto cleanup;

    // Find the file in the tree
    if (git_tree_entry_bypath(&entry, tree, path.c_str()) != 0)
        goto cleanup;

    // Ensure it's a blob (file)
    if (git_tree_entry_type(entry) != GIT_OBJECT_BLOB)
        goto cleanup;

    // Get the blob
    if (git_blob_lookup(&blob, repo, git_tree_entry_id(entry)) != 0)
        goto cleanup;

    // Extract contents
    {
        const void* data = git_blob_rawcontent(blob);
        size_t size = git_blob_rawsize(blob);

        result.assign((const char*)data, size);
    }
    cleanup:
        git_blob_free(blob);
    git_tree_entry_free(entry);
    git_tree_free(tree);
    git_commit_free(commit); // frees obj too
    // DO NOT git_object_free(obj) separately

    return result;
}