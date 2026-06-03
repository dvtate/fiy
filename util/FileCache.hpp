//
// Created by tate on 7/31/25.
//

#pragma once

#include <string_view>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <concepts>
#include <flat_map>
#include <deque>

#include "MMFile.hpp"

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

/**
 * Concatenate components into a single string, reserving space before appending them
 * @tparam Args char or StringViewLike
 * @param args parts to combine into single string
 * @return concatenated string
 */
template<typename... Args>
__attribute__((always_inline))
constexpr inline std::string concat(const Args&... args) {
    constexpr auto to_sv = []<typename T>(const T& v) constexpr {
        if constexpr (std::is_same_v<T, char>)
            return std::string_view(&v, 1);
        else
            return std::string_view(v);
    };
    return [](const auto ...sv) constexpr {
        std::string ret;
        ret.reserve((sv.size() + ...));
        (ret.append(sv), ...);
        return ret;
    }(to_sv(args)...);
}

/*
 * Cache frequently used file contents into static memory.
 *
 * Includes a rudimentary template engine.
 */
// TODO maybe non-templated base class that
template <auto(*GetBaseDirFunctor)(void)>
struct FileCache {

    // TODO should use map/unordered_map instead
    using ReplacementMap = std::vector<std::pair<std::string, std::string>>;

    static std::string load_file_as_string(const std::string& file_path) {
        return ::load_file_as_string(file_path);
    }

    static const std::string& prefix() {
        static auto ret = GetBaseDirFunctor();
        return ret;
    }

    static std::string path(const std::string& subpath) {
        return prefix() + subpath;
    }
    static std::string path(const char* subpath) {
        return prefix() + subpath;
    }

    /// Get cached contents of file as a string
    template<const char* FileSubPath>
    static const std::string& file_contents() {
        static const std::string contents = load_file_as_string(prefix() + FileSubPath);
        return contents;
    }

    /// Can be used similar to file_contents but uses mmap
    /// Good for large files: lets OS free up RAM
    /// No Global substitutions
    template<const char* FileSubPath>
    static const MMFile& mm_file() {
        static const MMFile file{path(FileSubPath)};
        return file;
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
    template<class K, class V>
    [[nodiscard]] static std::string
    mustache(std::string template_string, const std::vector<std::pair<K, V>>& rules) {
        // Editing in-place to reduce memory consumption
        // Probably a more performant way to do this
        std::size_t i = 0;
        while ((i = template_string.find("{{", i)) != std::string::npos) {
            const std::size_t end = template_string.find("}}", i + 2);
            const auto tag = std::string_view(template_string
                ).substr(i + 2, end - (i + 2));
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

    /**
     * Replace all {{tags}} in template_string with corresponding replacements from rules
     * @param template_string mustache template string
     * @param rules replacements to apply
     * @return new string with replacements
     * @remark tags not found in rules will not be removed!
     */
    [[nodiscard]] static std::string mustache(
        const std::string_view template_string,
        const std::flat_map<std::string_view, std::string_view>& rules
    ) {
        using Replacement = std::flat_map<std::string_view, std::string_view>::const_iterator;

        // index of {{ , key + value
        std::vector<std::pair<size_t, Replacement>> replacements;
        replacements.reserve(rules.size()); // reasonable starting point

        std::size_t len = template_string.size();
        std::size_t i = 0;
        while ((i = template_string.find("{{", i)) != std::string::npos) {
            const std::size_t start = i + 2;
            const std::size_t end = template_string.find("}}", start);
            const auto tag = template_string.substr(start, end - start);
            auto it = rules.find(tag);
            if (it != rules.end()) {
                replacements.emplace_back(i, it);
                len -= 4; // {{ }}
                len -= tag.size();
                len += it->second.size();
#ifdef FIY_DEBUG
            } else {
                // No replacement, leave it
                std::cerr <<"FileCache::mustache(): Unknown tag: {{" << tag <<"}}\n";
#endif
            }

            // put i after the }}
            i = end + 2;
        }

        std::string ret;
        ret.reserve(len);
        i = 0;
        for (const auto& p : replacements) {
            const std::size_t l = p.first - i;
            ret.append(template_string, i, l);
            ret.append(p.second->second);
            i += l;
            i += 2; // }}
        }
        ret.append(template_string, i, -1);
        return ret;
    }

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

namespace detail {
    /// Call default constructor of for given type
    template <std::default_initializable T>
    __attribute__((always_inline))
    constexpr T construct_default() {
        return {};
    }
}

using RootFileCache = FileCache<detail::construct_default<std::string>>;