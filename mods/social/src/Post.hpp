//
// Created by tate on 3/16/26.
//

#pragma once

#include <string>
#include <vector>
#include <ctime>


/// Way of identifying a post
struct PostId {
    /// Either local or remote id depending on if the domain field is empty or not
    int64_t id;

    /// Domain the post originates from
    std::string domain{""};

    bool is_local() const { return domain.empty(); }
};

/// Basic information about a post
struct Post {
    PostId id;

    /// User that created the post
    std::string username;

    /// timestamp the post was created at
    time_t created_ts;

    /// Full, raw content of the post
    std::string content;

    /// How many likes
    int likes_count;

    /// How many comments
    int comments_count;
};