//
// Created by tate on 7/31/25.
//

#ifndef FEDIY_FILECACHE_HPP
#define FEDIY_FILECACHE_HPP

#include <string>
#include <fstream>
#include <iostream>
#include <vector>


#if __has_include("../protocol/defs.hpp")
#include "../protocol/defs.hpp"
#else
#include <stdexcept>
#endif


template <auto(*GetBaseDirFunctor)(void)>
struct FileCache {

    using ReplacementMap = std::vector<std::pair<std::string, std::string>>;

    /// Load entire file contents into a string
    /// returns "" if file could not be opened
    static std::string load_file_as_string(const std::string& file_path) {
        // Open file
        std::ifstream in(file_path, std::ios::in | std::ios::binary);
        if (!in) [[unlikely]] {
            std::string msg = "Error: Could not open file ";
            msg += file_path;
#ifdef LOG_ERR
            LOG_ERR(msg);
#else
            throw new std::runtime_error(msg);
#endif

            return ""; // Return an empty string on failure
        }

        // Prepare buffer to store file contents
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());

        // Read file
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return contents;
    }

    /// Getter
    static const std::string& prefix() {
        static auto ret = GetBaseDirFunctor();
        return ret;
    }

    /// Get cached contents of file as a string
    template<const char* FileSubPath>
    static const std::string& file_contents() {
        static const std::string contents = load_file_as_string(prefix() + FileSubPath);
        return contents;
    }

    /// Get cached contents of file as a string with global replacements
    template<const char* FileSubPath, class Replacements>
    static const std::string& file_contents(
        const Replacements& replacements
    ) {
        static const std::string contents = replace_all(
            load_file_as_string(prefix() + FileSubPath),
            replacements
        );
        return contents;
    }

    static std::string replace_one(std::string haystack, const std::string_view needle, const std::string_view replacement) {
        std::size_t i = haystack.find(needle);
        if (i == std::string::npos)
            return haystack;
        haystack.replace(i, needle.size(), replacement);
        return haystack;
    }

    static std::string replace_all(std::string haystack, const std::string_view needle, const std::string_view replacement) {
        std::size_t i = 0;
        while ((i = haystack.find(needle, i)) != std::string::npos) {
            haystack.replace(i, needle.size(), replacement);
            i += replacement.size();
        }
        return haystack;
    }

    template <class Replacements>
    static std::string replace_all(
        std::string template_string,
        const Replacements& replacements
    ) {
        for (const auto& [ from, to ] : replacements)
            template_string = replace_all(std::move(template_string), from, to);
        return template_string;
    }

    /// Get cached contents of file as a string
    template<class Replacements>
    [[nodiscard]] static std::string
    mustache(std::string template_string, const Replacements& rules) {
        std::size_t i = 0;
        while ((i = template_string.find("{{", i)) != std::string::npos) {
            i += 2;
            const std::size_t end = template_string.find("}}", i);
            const auto tag = template_string.substr(i, end - i);
            for (const auto& [ needle, replacement ] : rules)
                if (tag == needle)
                    template_string.replace(i - 2, needle.size() + 4, replacement);
            i = end + 2;
        }
        return template_string;
    }

    // TODO sanitize html (ie - to prevent XSS)
};

#endif //FEDIY_FILECACHE_HPP
