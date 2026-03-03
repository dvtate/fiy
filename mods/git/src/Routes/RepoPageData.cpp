//
// Created by tate on 2/26/26.
//

#include "RepoPageData.hpp"

#include <ctime>
#include <iomanip>

std::string RepoFileBrowserPageData::time_diff_str(const time_t then) {
    const time_t now = fiy::Host::info.now();
    const auto secs = abs(then - now);
    const std::string dist = now > then ? " ago" : " from now";

    // Use the biggest relevant unit of measure
    constexpr long min = 60;
    constexpr long hour = 60 * min;
    constexpr long day = 24 * hour;
    constexpr long week = 7 * day;
    constexpr long year = 52 * week;
    if (secs / year > 1)
        return std::to_string(secs / year) + " years" + dist;
    if (secs / week > 1)
        return std::to_string(secs / week) + " weeks" + dist;
    if (secs / day > 1)
        return std::to_string(secs / day) + " days" + dist;
    if (secs / hour > 1)
        return std::to_string(secs / hour) + " hours" + dist;
    if (secs / min > 1)
        return std::to_string(secs / min) + " minutes" + dist;
    return "just now";
}

/// Convert timestamp to RFC 3339 UTC time string
std::string RepoFileBrowserPageData::time_str(const time_t ts) {
    struct tm tm;
    if (gmtime_r(&ts, &tm) == nullptr) {
        return std::to_string(ts);
    }

    // TODO probably better way to do this
    std::stringstream ss;
    ss <<std::put_time(&tm, "%FT%TZ");
    return ss.str();
}

/**
 * Convert the entries list into HTML for the page
 */
std::string RepoFileBrowserPageData::entries_html() const {
    // This is probably really slow
    std::string ret;
    for (const auto& e : this->files) {
        ret += "<tr><td class=\"file-name\"><span class=\"file-icon\">";
        switch (e.type) {
            case GitRepo::Entry::DIRECTORY:
                ret += "\xf0\x9f\x93\x81"; // folder emoji
                break;
            case GitRepo::Entry::COMMIT:
                // TODO find something better
                ret += "\xf0\x9f\x93\x8d"; // pushpin emoji
                break;
            case GitRepo::Entry::FILE:
                ret += "\xf0\x9f\x93\x84"; // file emoji
                break;
        }
        ret += "</span> ";
        ret += e.path;
        ret += "</td><td class=\"file-commit-msg\"><a href=\"";
        ret += this->name;
        ret += "/commit/";
        ret += e.last_commit.id;
        ret += "\">";
        ret += e.last_commit.message.substr(0,
            e.last_commit.message.find('\n'));
        ret += "</a></td><td class=\"file-time\" title=\"";
        ret += time_str(e.last_commit.ts);
        ret +="\">";
        ret += time_diff_str(e.last_commit.ts);
        ret += "</td></tr>\n";
    }
    return ret;
}
