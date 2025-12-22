//
// Created by tate on 12/3/25.
//

#ifndef FEDIY_PAGES_HPP
#define FEDIY_PAGES_HPP

#include <utility>

#include "Repo.hpp"
#include "../../../modlib/fediymod.hpp"
#include "../../../util/FileCache.hpp"

inline std::string get_frontend_dir() {
    return fiy::Host::info.data_dir + std::string("/static");
}

struct Pages : FileCache<get_frontend_dir> {

    static ReplacementMap get_host_data() {
        return {
            { "{{fiy_domain}}", fiy::Host::info.domain },
            { "{{fiy_baseuri}}", fiy::Host::info.base_uri },
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

        static constexpr char  repo_create[] = "/repo_create.html";
        return FileCache::mustache(
            Pages::file_contents<repo_create>(),
            ReplacementMap({
            { "fiy_user", user },
            { "orgs_option_list", orgs_option_list }
            })
        );
    }

    static std::string repo_page(const RepoInfo& repo) {
        static constexpr char repo_page[] = "/repo.html";
        return FileCache::mustache(
            Pages::file_contents<repo_page>(),
            ReplacementMap({
                { "repo_clone_url", fiy::Host::info.base_uri + std::string("/") + repo.path() },
                { "repo_breadcrumbs",  repo.path() },
                { "repo_description", repo.description },
            })
        );
    }
};

#endif //FEDIY_PAGES_HPP