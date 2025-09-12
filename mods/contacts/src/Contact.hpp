//
// Created by tate on 7/30/25.
//

#pragma once

#include <string>
#include <utility>
#include <vector>

struct VC {
    struct Prop {
        int64_t id{-1};
        std::string name, params, value;
        int8_t visibility{0}; // default to private
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

    /// 256x256 blue-ish square
    static constexpr const char* default_pfp = "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAQAAAAEAAQMAAABmvDolAAAAA1BMVEW10NBjB"
        "BbqAAAAH0lEQVRoge3BAQ0AAADCoPdPbQ43oAAAAAAAAAAAvg0hAAABmmDh1Q"
        "AAAABJRU5ErkJggg==";

    /// Convert to vCard text
    std::string to_vcard();

    bool parse(std::string vc);

    static VC basic_profile(const std::string& user);

    [[nodiscard]] bool invalid() const {
        return id == -1 && update_ts == 0;
    }
};