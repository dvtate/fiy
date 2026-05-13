//
// Created by tate on 12/3/25.
//

#pragma once

#include <boost/core/data.hpp>

#include "RepoPageData.hpp"
#include "../Repos/LocalRepo.hpp"
#include "../../../../modlib/fiymod.hpp"
#include "../../../../util/FileCache.hpp"

inline std::string get_frontend_dir() {
    return fiy::host().data_dir + std::string("/static");
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


    static ReplacementMap get_host_data() {
        return {
            { "{{fiy_domain}}", fiy::host().domain },
            { "{{fiy_baseuri}}", fiy::host().base_uri },
        };
    }

    /// Automatically replace host_data in the loaded template
    template<const char* FileSubPath>
    static const std::string& file_contents() {
        return FileCache<get_frontend_dir>::file_contents<FileSubPath>(get_host_data());
    }

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

    static std::string repo_create_page(
        const std::string& user,
        const std::vector<std::string>& orgs
    ) {
        std::string orgs_option_list;
        if (!orgs.empty()) {
            orgs_option_list += "<optgroup label=\"Organizations\">";
            for (const auto& org : orgs) {
                orgs_option_list += "<option value=\"";
                orgs_option_list += org;
                orgs_option_list += "\">";
                orgs_option_list += org;
                orgs_option_list += "</option>";
            }
        }

        static constexpr char repo_create[] = "/repo_create.html";
        return FileCache::mustache(
            Pages::file_contents<repo_create>(),
            ReplacementMap({
            { "fiy_user", user },
            { "orgs_option_list", orgs_option_list }
            })
        );
    }

    // This isn't gonna work...
    // Edge cases not handled:
    //  - user not logged in but still shows profile tab
    //  - repo not initialized (ie - no files), should give new repo instructions
    // TODO instead put page data into JSON object and render it with JS on client ?
    static std::string repo_page(const RepoPageData& repo, const char* request_user = nullptr) {
        // TODO the right way to do this is probably to populate the page data by calling the API
        //      that we already have to implement in order for federation to work.
        static constexpr char repo_page[] = "/repo.html";
        auto pfp_url = [](const std::string& user) {
            return fiy::host().host_base_uri() + "/mods/contacts/pfp/" + user;
        };
        static std::string visibility_strs[] = {
            "Private", "Instance private", "Federated", "Public"
        };
        return FileCache::mustache(
            Pages::file_contents<repo_page>(),
            ReplacementMap({
                { "repo_owner_pfp", pfp_url(repo.owner_user()) },
                { "last_commit_author_pfp", pfp_url(repo.last_commit.author.local_user()) },
                { "last_commit_author", repo.last_commit.author.profile_link() },
                { "last_commit_time", time_str(repo.last_commit.ts) },
                { "last_commit_time_diff", time_diff_str(fiy::host().now(), repo.last_commit.ts) },
                { "last_commit_id", repo.last_commit.id },
                { "last_commit_msg", repo.last_commit.message.substr(0,
                    repo.last_commit.message.find('\n')) },
                { "mod_baseurl", fiy::host().base_uri },
                { "repo_branch", repo.active_branch },
                { "repo_branches_count", std::to_string(repo.branches_count) },
                { "repo_commits_count", std::to_string(repo.commits_count) },
                { "repo_description", repo.description },
                { "repo_forks_count", std::to_string(-1) },
                { "repo_likes_count", std::to_string(repo.likes_count) },
                { "repo_name", repo.name },
                { "repo_owner", repo.owner_user() },
                { "repo_tags_count", std::to_string(repo.commits_count) },
                // { "repo_tags_html", "" }, // descriptive tags ala hashtags
                { "repo_tickets_count", std::to_string(repo.tickets_count) },
                { "repo_visibility", visibility_strs[(size_t)repo.visibility] },
                { "repo_clone_url", fiy::host().base_uri + std::string("/") + repo.path() },
                { "repo_entries_html", repo.entries_html() },
                { "request_user", request_user == nullptr ? "" : request_user },
                { "fiy_domain", fiy::host().domain }
            })
        );
        // TODO future: contributors, languages, readme viewer,
    }

    static std::string user_page(const std::string& user, const char* request_user = nullptr) {
        static constexpr char user_page[] = "/user.html";

        return FileCache::mustache(
            Pages::file_contents<user_page>(),
            ReplacementMap({
                { "fiy_user", user },
                { "fiy_domain", fiy::host().domain },
                { "request_user", request_user == nullptr ? "" : request_user },
                { "mod_baseurl", fiy::host().base_uri },
                { "fiy_display_name", user },
                { "fiy_user_bio", "" }
            })
        );
    }
};
