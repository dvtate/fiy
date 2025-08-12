#pragma once

#include <git2.h>

// Wrapper around libgit2.so




class GitRepo {

    void create() {
        
        git_repository* repo = nullptr;
        // struct opts o = {};
        git_threads_init();

    }
};