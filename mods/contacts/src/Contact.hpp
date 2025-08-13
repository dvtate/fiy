//
// Created by tate on 7/30/25.
//

#pragma once

#include <string>
#include <utility>
#include <vector>

struct VC {
    struct Prop {
        int64_t id;
        std::string name, params, value;
    };

    /// Unique ID
    int64_t id{-1};

    /// fediy user that owns the vcard
    std::string owner;

    /// fediy user represented by the card
    std::string user;

    /// last edited timestamp
    time_t update_ts{0};

    /// vCard properties
    std::vector<Prop> props;

    /// Convert to vCard text
    std::string to_vcard();

    bool parse(const std::string& vc);
};