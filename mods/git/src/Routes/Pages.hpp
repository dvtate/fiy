//
// Created by tate on 12/3/25.
//

#pragma once

#include <boost/core/data.hpp>

#include "RepoPageData.hpp"
#include "../Repos/LocalRepo.hpp"
#include "../../../../modlib/fiymod.hpp"
#include "../../../../util/FileCache.hpp"

inline const std::string& get_frontend_dir() {
    static const std::string ret = fiy::host().data_dir + std::string("/static");
    return ret;
}

struct Pages : FileCache<get_frontend_dir> {
    static std::string time_diff_str(const time_t now, const time_t then) {
        const auto secs = abs(then - now);
        const std::string dist = now > then ? " ago" : " from now";

        // Use the biggest relevant unit of measure
        constexpr long min = 60;
        constexpr long hour = 60 * min;
        constexpr long day = 24 * hour;
        constexpr long week = 7 * day;
        constexpr long month = 30 * day;
        constexpr long year = 52 * week;
        if (secs / year > 1)
            return std::to_string(secs / year) + " years" + dist;
        if (secs / year == 1)
            return now > then ? "last year" : "next year";
        if (secs / month > 1)
            return std::to_string(secs / month) + " months" + dist;
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
    static std::string time_str(const time_t ts) {
        struct tm tm;
        if (gmtime_r(&ts, &tm) == nullptr) {
            return std::to_string(ts);
        }

        // TODO probably better way to do this
        std::stringstream ss;
        ss <<std::put_time(&tm, "%FT%TZ");
        return ss.str();
    }

    static std::string pre_render_host_data(const std::string& templ);

    /// Automatically replace host_data in the loaded template
    template<const char* FileSubPath>
    static const std::string& file_contents() {
        static const std::string ret = pre_render_host_data(FileCache::get_file_contents<FileSubPath>());
        return ret;
    }

    // Only global replacements
    template<const char* FileSubPath>
    static fiy::Body file_body() {
        return fiy::Body(file_contents<FileSubPath>());
    }

    // TODO make version of this which handles MMFile::Error and gives 404/500
    template<const char* FileSubPath>
    static fiy::Body mm_file_body() {
        return fiy::Body(std::string_view(
            mm_file<FileSubPath>()));
    }

    static std::string repo_create_page(std::string_view user, const std::vector<std::string>& orgs);
    static std::string user_page(std::string_view user, const char* request_user = nullptr);
    static std::string repo_page(const RepoPageData& repo, const char* request_user = nullptr);
    static std::string landing_page(std::string_view profile_url);
};
