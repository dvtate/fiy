//
// Created by tate on 3/4/26.
//

#pragma once

#include <deque>
#include <string>
#include <vector>

/*
 * always on the left:
 * - pfp
 * - name
 * - bio
 * - "view profile" button -> contacts
 * - followers/following
 * - follow / following
 *
 * subpages:
 * - user summary
 * - user activity
 * - user connections
 * - user hearts
 * - user settings
 */

struct UserBasicProfile {
    std::string fiy_user; // also used to get their pfp
    std::string display_name;
    std::string bio;
    int followers_count{-1};
    int following_count{-1};
    int likes_count{-1};
};

// Includes data shown in a list of repos
struct UserBasicRepo {
    std::string path;
    std::string description;
    int likes_count{-1};
    // TODO forks_count, license, language, tags, etc.

};

struct UserBasicContribution {
    std::string repo_path;
    time_t ts;
    enum Type {
        Commit,
        PR,
        Issue,
        Other
    } type;

    bool is_private() const {
        return repo_path.empty();
    }
};
struct UserBasicContributions {
    std::deque<UserBasicContribution> contributions;

    std::string to_json();
};

struct UserOverview {
    std::vector<UserBasicRepo> pinned_repos;
};

struct UserPageData {

    // un
    // dn
    // username
    // bio
    //
};