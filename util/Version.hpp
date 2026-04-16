//
// Created by tate on 4/16/26.
//

#pragma once

#include <string>
#include <string_view>

/**
 * Semver-inspired version struct
 */
struct Version {
    /// Major version
    // using long in case this is a date/ts
    mutable long major{-1};

    /// Minor version
    mutable long minor{-1};

    /// Cached Major version string
    mutable std::string major_string;

    Version() = default;

    explicit Version(const std::string& version_str) {
        std::size_t pos;
        major = std::stoi(version_str, &pos); // TODO use from_chars
        major_string = std::to_string(major);
        if (pos >= version_str.size() || version_str.at(pos) != '.') {
            minor = 0;
            return;
        }
        char* end = nullptr;
        minor = std::strtol(version_str.c_str() + pos, &end, 10);
    }

    // TODO
    // explicit Version(const std::string_view& version_str) {}

    Version(const long major, const long minor):
            major(major), minor(minor)
    {
        major_string = std::to_string(major);
    }

    [[nodiscard]] bool compatible(const Version& other) const {
        return major == other.major;
    }

    [[nodiscard]] bool compatible(const std::string& other_major_str) const {
        return major_string == other_major_str;
    }

    [[nodiscard]] auto operator<=>(const Version& other) const {
        if (const auto c = major <=> other.major; c != 0)
            return c;
        return minor <=> other.minor;
    }

    [[nodiscard]] std::string str() const {
        return major_string + '.' + std::to_string(minor);
    }

    [[nodiscard]] bool initialized() const {
        return major != -1;
    }
};