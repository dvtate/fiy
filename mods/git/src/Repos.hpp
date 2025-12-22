//
// Created by tate on 12/8/25.
//

#pragma once

#include <vector>
#include <string>
#include <string_view>

#include "../../../modlib/fediymod.hpp"

#include "Repo.hpp"

struct Repos {
    enum class Access {
        Blocked = 0,
        Read = 1,
        Write = 2,
        Admin = 3,
        Owner = 4,
        Error
    };

    /**
     * Convert repo path to cannonical form
     * @param url repo string of form
     *      [/]user[@instance.xyz]/repo[.git][/subpath/...][?query=sting]
     * @return canonical representation of form
     *          user[@remote.instance]/repo
     */
    static std::string canonical_url(std::string url) {
        // shortest url: a/b
        if (url.size() < 3)
            return {};

        // Remove leading ////'s
        if (url[0] == '/')
            url = url.substr(url.find_first_not_of('/'));
        if (url.size() < 3)
            return {};

        // Invalid
        const auto slash = url.find('/');
        if (slash == std::string::npos)
            return {};

        // Remove local domain
        const auto at = url.find('@');
        if (at != std::string::npos) {
            // Invalid
            if (at > slash)
                return {};

            const auto instance = url.substr(at, slash - at);
            if (instance == fiy::Host::info.domain) {
                // user@local/repo[.git] --> user/repo[.git]
                url = url.substr(0, at) + url.substr(slash);
            }
        }

        // Remove .git
        if (url.ends_with(".git"))
            url = url.substr(0, url.size() - 4);

        return url;
    }
};