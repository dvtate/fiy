//
// Created by tate on 7/31/25.
//

#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#if __has_include("../protocol/defs.hpp")
#include "../protocol/defs.hpp"
#else
#include <stdexcept>
#endif

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

/*
 * Cache frequently used file contents into static memory.
 *
 * Includes a rudimentary template engine.
 */

template <auto(*GetBaseDirFunctor)(void)>
struct FileCache {

    // TODO should use map/unordered_map instead
    using ReplacementMap = std::vector<std::pair<std::string, std::string>>;

    static std::string load_file_as_string(const std::string& file_path) {
        return ::load_file_as_string(file_path);
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
        // Editing in-place to reduce memory consumption
        // Probably a more performant way to do this
        std::size_t i = 0;
        while ((i = template_string.find("{{", i)) != std::string::npos) {
            const std::size_t end = template_string.find("}}", i + 2);
            const auto tag = template_string.substr(i + 2, end - (i + 2));
            for (const auto& [ needle, replacement ] : rules)
                if (tag == needle) {
                    template_string.replace(i, needle.size() + 4, replacement);
                    i += replacement.size();
                    goto match_found;
                }
            i = end + 2;
match_found:
            ;
        }
        return template_string;
    }
//
//     /**
//      * Subset of Mustache templating engine that only handles simple replacements
//      * @tparam VecType list of replacement mappings
//      * @tparam T list entry type, probably a pair of strings
//      * @param template_string mustache template
//      * @param rules list of replacement mappings
//      * @param remove_unused if invalid variable
//      * @return populated template
//      */
//     template<
//         template<typename T> class VecType,
//         typename T = std::pair<std::string_view, std::string>
//     >
//     [[nodiscard]] static std::string
//     mustache(
//         std::string template_string,
//         const VecType<T>& rules,
//         const bool remove_unused = false
//     ) {
//         // Editing in-place to reduce memory consumption
//         // Probably a more performant way to do this
//         std::size_t i = 0;
//         while ((i = template_string.find("{{", i)) != std::string::npos) {
//             i += 2;
//             const std::size_t end = template_string.find("}}", i);
//             const auto tag = template_string.substr(i, end - i);
//             typename T::second_type* value = nullptr;
//             for (const auto& [ needle, replacement ] : rules)
//                 if (tag == needle)
//                     value = &replacement;
//
//             if (value != nullptr || remove_unused)
//                 template_string.replace(
//                     i - 2,
//                     end - i + 4,
//                     value == nullptr ? "" : *value
//                 );
// #ifdef FIY_DEBUG
//             if (value == nullptr && remove_unused)
// #endif
//             i = end + 2;
//         }
//         return template_string;
//     }

    /**
     * Escape HTML characters in a string
     * @param non_html String to escape HTML characters in
     * @return string with HTML characters escaped
     */
    static std::string escape_html(const std::string& non_html) {
        std::string ret;
        // note could probably pre-reserve an expected growth ratio
        ret.reserve(non_html.size());
        for (const char c : non_html) {
            switch (c) {
                case '<':
                    ret += "&lt;";
                    break;
                case '&':
                    ret += "&amp;";
                    break;
                case '>':
                    ret += "&gt;";
                    break;
                case '\'':
                    ret += "&apos;";
                    break;
                case '\"':
                    ret += "&quot;";
                    break;
                default:
                    ret += c;
            }
        }
        return ret;
    }
};
