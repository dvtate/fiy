//
// Created by tate on 7/30/25.
//

#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <string_view>

#include "../../../modlib/fediymod.hpp"

#include "Contact.hpp"
#include "DB.hpp"

// TODO replace this with FileCache?

namespace Pages {
    static std::string full_path(const std::string& subpath) {
        return std::string(fiy::Host::info.data_dir) + "/assets/" + subpath;
    }

    //////
    // Simple Templating engine
    /////

    /**
     * Read the entire contents of given file path into a string
     * @param file_path path to file
     * @return contents of file
     */
    std::string load_file_as_string(const std::string& file_path) {
        // Open the file
        std::ifstream f{file_path};
        if (!f.is_open()) {
            std::string log_msg = "Error: Could not open file ";
            log_msg += file_path;
            fiy::log_error(log_msg);
            return ""; // Return an empty string on failure
        }

        // Create a string big enough for the file
        std::string ret;
        f.seekg(0, std::ios::end);
        ret.reserve(1 + (ssize_t) f.tellg());
        f.seekg(0, std::ios::beg);

        // Read entire file content into return string
        ret.assign(std::istreambuf_iterator<char>(f),
                   std::istreambuf_iterator<char>());
        return ret;
    }

    /**
     * Get cached contents of file as a string
     * @tparam FileSubPath static const char* data dir subpath
     * @return string contents of the file
     */
    template<const char* FileSubPath>
    const std::string& file_contents() {
        static const std::string contents = load_file_as_string(full_path(FileSubPath));
        return contents;
    }
    template<const char* FileSubPath>
    fiy::Body file_body() {
        return fiy::Body(file_contents<FileSubPath>());
    }

    /// Primitive templating engine
    inline std::string replace_one(std::string haystack, const std::string_view needle, const std::string_view replacement) {
        std::size_t i = haystack.find(needle);
        if (i == std::string::npos)
            return haystack;
        haystack.replace(i, needle.size(), replacement);
        return haystack;
    }

    inline std::string replace_all(std::string haystack, const std::string_view needle, const std::string_view replacement) {
        std::size_t i = 0;
        while ((i = haystack.find(needle, i)) != std::string::npos) {
            haystack.replace(i, needle.size(), replacement);
            i += replacement.size();
        }
        return haystack;
    }

    inline std::string replace_all(
        std::string template_string,
        const std::vector<std::pair<std::string_view, std::string_view>>& replacements
    ) {
        for (const auto [ from, to ] : replacements)
            template_string = replace_all(std::move(template_string), from, to);
        return template_string;
    }

    //////
    // Cached Static Pages
    //////

    inline fiy::Body main_js() {
        static const std::string contents = Pages::replace_all(
            load_file_as_string(full_path("main.bundle.js")),
            {
                {   "{{fediy_contacts_base_uri}}",
                    fiy::Host::info.base_uri
                }, {
                    "{{fediy_contacts_domain}}",
                    fiy::Host::info.domain
                }
            }
        );
        return fiy::Body(contents);
    }

    inline fiy::Body index_html() {
        static std::string contents = replace_all(
            load_file_as_string(full_path("index.html")),
            {
                { "{{fediy_contacts_base_uri}}", fiy::Host::info.base_uri },
                { "{{fediy_contacts_domain}}", fiy::Host::info.domain }
            }
        );
        return fiy::Body(contents);
    }

    inline fiy::Body main_css() {
        static const char path[] = "main.css";
        return fiy::Body(file_contents<path>());
    }
}
