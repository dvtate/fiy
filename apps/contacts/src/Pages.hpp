//
// Created by tate on 7/30/25.
//

#pragma once

#include <string>
#include <fstream>
#include <string_view>

#include "../../../modlib/fediymodpp.hpp"

#include "Contact.hpp"
#include "DB.hpp"

extern const fiy_host_info_t* g_host_info;

namespace Pages {

    std::string load_file_as_string(std::string&& file_path) {
        // Open the file
        std::ifstream f{file_path};
        if (!f.is_open()) {
            std::string log_msg = "Error: Could not open file ";
            log_msg += file_path;
            g_host_info->log(1, log_msg.c_str());
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

    /// Get cached contents of file as a string
    template<const char* FileSubPath>
    const std::string& file_contents() {
        static const std::string contents = load_file_as_string(
            std::string(g_host_info->data_dir) + FileSubPath
        );
        return contents;
    }

    /// Primitive templating engine
    std::string replace_one(std::string haystack, const std::string_view& needle, const std::string_view& replacement) {
        std::size_t i = haystack.find(needle);
        if (i == std::string::npos)
            return haystack;
        haystack.replace(i, needle.size(), replacement);
        return haystack;
    }
    std::string replace_all(std::string haystack, const std::string_view& needle, const std::string_view& replacement) {
        std::size_t i = 0;
        while ((i = haystack.find(needle, i)) != std::string::npos) {
            haystack.replace(i, needle.size(), replacement);
            i += replacement.size();
        }
        return haystack;
    }

    std::string index_html(const std::string& user) {
        static const char path[] = "/index.html";
        return replace_one(
            file_contents<path>(),
            "{{contacts_json}}",
            Contact::json_list(DB::get_contacts(user))
        );
    }
};
