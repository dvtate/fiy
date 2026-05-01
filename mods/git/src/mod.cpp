//
// Created by tate on 12/2/25.
//

#include <git2.h>
#include <string_view>

#include "../../../modlib/fiymod.hpp"

#include "../../../util/WebUtils.hpp"

#include "Routes/Router.hpp"

void delete_user(const char* username) {
    fiy::log_warning("Git module currently does not handle user deletion");
    // TODO delete their repos, interactions, etc.
    if (strchr(username, '@') != nullptr) {
        // TODO delete RepoAccess entries
        // TODO delete OrgMembers entries
        return;
    }

    // Delete user from database
    // Delete user's repos folder
}

/// Export: Start module
FIY_EXPORT fiy::ModInfo* start(const fiy_host_info_t* host_info) {
    // Initialize libgit2
    if (git_libgit2_init() < 0) {
        const git_error *e = giterr_last();
        std::string message = "Failed to initialize libgit2: ";
        message += e->message;
        host_info->log(
            (int)fiy::Host::Log::FATAL,
            message.c_str());
        return nullptr;
    }

    // TODO verify database and directory structure
    // TODO fix database and directory structure if needed

    // Exchange info
    static fiy::ModInfo mod_info = {
        .on_request = handle_request,
        .delete_user = delete_user,
        .id="git",
        .version = "0.0"
    };
    fiy::host() = *host_info;
    return &mod_info;
}

/// Export: Stop module
FIY_EXPORT void stop() {
    // Shutdown libgit2
    if (git_libgit2_shutdown() < 0) {
        const git_error *e = giterr_last();
        std::string msg = "Failed to shutdown libgit2: ";
        msg += e->message;
        fiy::log_error(msg);
    }
}