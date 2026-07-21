//
// Created by tate on 3/10/26.
//
//
// Primitive template engine with optimization for constexpr keys
//

#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <algorithm>
#include <utility>
#include <array>
#include <cassert>

namespace MinSSR {
    template<size_t N>
    using Tags = std::array<std::string_view, N>;

    template<std::ranges::random_access_range R>
    consteval R sort_tags(R&& tags) {
        std::ranges::sort(tags);
        return tags;
    }

    template<std::ranges::random_access_range R>
    constexpr void assert_sorted(const R& tags) {
#if __cplusplus >= 202302L
        if consteval {
            if (!std::is_sorted(tags.cbegin(), tags.cend()))
                throw "tags not sorted";
        } else
#endif
        {
            assert(std::is_sorted(tags.cbegin(), tags.cend()));
        }
    }

    template<std::ranges::random_access_range R>
    constexpr size_t index(
        const R& sorted_tags,
        const std::string_view tag
    ) {
#if __cplusplus >= 202302L
        if consteval {
            if (!std::is_sorted(sorted_tags.cbegin(), sorted_tags.cend()))
                throw "tags not sorted";
        }
#endif

        // Binary search
        auto it = std::ranges::lower_bound(sorted_tags, tag);

        // Not found
        if (it == sorted_tags.end() || *it != tag)
            return sorted_tags.size();

        // Return index
        return std::distance(sorted_tags.begin(), it);
    }

    // Don't implicitly convert std::strings that could go out of scope
    template<std::ranges::random_access_range R>
    inline void set_index(R& tags, const size_t index, const std::string_view value) {
        tags[index] = value;
    }
    template<std::ranges::random_access_range R>
    inline void set_index(R& tags, const size_t index, const char* value) {
        tags[index] = std::string_view(value);
    }
    template<std::ranges::random_access_range R>
    inline void set_index(R& tags, const size_t index, std::string&& value) = delete;

    /**
     * Replace all {{tags}} in template_string with corresponding substitutions
     * @param template_string mustache template string
     * @param sorted_tags sorted array of tags to replace with corresponding substitutions
     * @param substitutions replacements to apply,
     *      if one larger than sorted_tags, the last value will replace unknown values
     * @return new string with replacements
     * @remark tags with index greater than
     */
    template<
        std::ranges::random_access_range R1,
        std::ranges::random_access_range R2>
    [[nodiscard]] constexpr std::string mustache(
        const std::string_view template_string,
        const R1& sorted_tags,
        const R2& substitutions
    ) {
        // index of {{ , key + value
        std::vector<std::pair<size_t, ssize_t>> replacements;
        replacements.reserve(sorted_tags.size()); // reasonable starting point

        std::size_t len = template_string.size();
        std::size_t i = 0;
        while ((i = template_string.find("{{", i)) != std::string::npos) {
            const std::size_t start = i + 2;
            const std::size_t end = template_string.find("}}", start);
            if (end == std::string_view::npos) [[unlikely]] {
                break;
            }
            const auto tag = template_string.substr(start, end - start);
            const auto idx = index(sorted_tags, tag);
            if (idx < substitutions.size()) {
                replacements.emplace_back(i, idx == sorted_tags.size() ? -tag.size() - 1 : idx);
                len -= 4; // {{ }}
                len -= tag.size();
                len += substitutions[idx].size();
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
            if (p.second < 0)
                ret.append(substitutions[substitutions.size() - 1]);
            else
                ret.append(substitutions[p.second]);
            i += l;
            i += 4; // {{}}
            if (p.second < 0)
                i -= p.second + 1;
            else
                i += sorted_tags[p.second].size();
        }
        ret.append(template_string, i, -1);
        return ret;
    }

    /// When the template is constant we can remember where replacements need to occur
    struct ParsedTemplate {
        /// Template string with template params removed
        std::string stripped_template;

        /// Locations + substitution index to replace in the template string
        std::vector<std::pair<size_t, ssize_t>> substitution_points;

        /**
         * Populate the parsed template with the corresponding runtime values
         * @param substitutions list of corresponding replacements w/ default handler at the end
         * @return replaced string
         * @remark unknown value must be either the last element of the array or the template must not
         *  have any unknown values (ie - parsed with leave_unknown = true)
         */
        template<std::ranges::random_access_range R>
        constexpr std::string process(const R& substitutions) const {
            // Calculate size
            size_t len = stripped_template.size();
            for (const auto& p : substitution_points)
                if (p.second < 0)
                    len += substitutions[substitutions.size() - 1].size();
                else
                    len += substitutions[p.second].size();

            // Construct string
            std::string ret;
            ret.reserve(len);
            size_t prev = 0;
            for (const auto& p : substitution_points) {
                const auto l = p.first - prev;
                ret.append(stripped_template, prev, l);
                if (p.second < 0)
                    ret.append(substitutions[substitutions.size() - 1]);
                else
                    ret.append(substitutions[p.second]);
                prev = p.first;
            }
            ret.append(stripped_template, prev);
            return ret;
        }

        /**
         * Parse a template
         * @param template_string input template
         * @param sorted_tags sorted array of tags used in the template
         * @param leave_unknown will unknown tags be ignored (true) or replaced (false)?
         * @return
         */
        template<std::ranges::random_access_range R>
        static inline constexpr
        ParsedTemplate parse_mustache(
            const std::string_view template_string,
            const R& sorted_tags,
            const bool leave_unknown = true
        ) {
            // TODO improved algorithm: copy+edit template string

            // Find all substitution points in template string
            ParsedTemplate ret;
            size_t prev = 0;
            size_t i = 0;
            size_t index_offset = 0;
            while ((i = template_string.find("{{", prev)) != std::string::npos) {
                index_offset += i - prev;

                const size_t start = i + 2;
                const size_t end = template_string.find("}}", start);
                if (end == std::string_view::npos) [[unlikely]] {
                    index_offset += template_string.size() - i;
                    break;
                }
                const auto l = end - start;
                const auto tag = template_string.substr(start, l);
                const auto idx = index(sorted_tags, tag);

                if (idx == sorted_tags.size()) {
                    if (leave_unknown) {
                        // Not a substitution, include tag
                        index_offset += l + 4;
                    } else {
                        // Replace unknown
                        ret.substitution_points.emplace_back(index_offset, -1 - (ssize_t)l); // default handler
                    }
                } else {
                    // Substitution
                    ret.substitution_points.emplace_back(index_offset, idx);
                }

                // Put i after the }}
                prev = end + 2;
            }

            // TODO resize_and_overwrite
            ret.stripped_template.reserve(index_offset);
            i = 0;              // index in original template string
            index_offset = 0;   // index in stripped string
            for (auto& p : ret.substitution_points) {
                // End of last -> start of current
                const std::size_t l = p.first - index_offset;

                // Copy non-param part of template
                ret.stripped_template.append(template_string, i, l);

                // Don't copy param into stripped string
                index_offset += l;

                // Translated index
                i += l; // non-param part
                i += 4; // {{}}
                i += p.second < 0
                    ? -p.second - 1
                    : sorted_tags[p.second].size();
            }

            // Include end
            if (i < template_string.size())
                ret.stripped_template.append(template_string, i);

            return ret;
        }
    };
}

/* usage:
#define USER_PAGE_RULES(KV) \
    KV("fiy_user", user) \
    KV("fiy_domain", fiy::host().domain) \
    KV("request_user", request_user == nullptr ? "" : request_user) \
    KV("mod_baseurl", fiy::host().base_uri) \
    KV("fiy_display_name", user) \
    KV("fiy_user_bio", "")

    return MIN_SSR_MUSTACHE(template_string, USER_PAGE_RULES);
 */

#define FIY_MIN_SSR_TAG_CSV(k, v) k,
#define FIY_MIN_SSR_SET_REPLACEMENT(k, v) MinSSR::set_index(replacements, MinSSR::index(tags, k), v);
#define FIY_MIN_SSR_TAGS_LEN(k, v) + 1

#define  MIN_SSR_TAGS(rules) (MinSSR::sort_tags(MinSSR::Tags< \
    0 rules(FIY_MIN_SSR_TAGS_LEN)>({rules(FIY_MIN_SSR_TAG_CSV)})));\

#define MIN_SSR_MUSTACHE(template_string, rules) \
    ([&](){ \
        static constexpr auto tags = (MinSSR::sort_tags(MinSSR::Tags< \
            0 rules(FIY_MIN_SSR_TAGS_LEN)>({rules(FIY_MIN_SSR_TAG_CSV)})));\
        static const auto tp = MinSSR::ParsedTemplate::parse_mustache(template_string, tags);\
        MinSSR::Tags<tags.size()> replacements; \
        rules(FIY_MIN_SSR_SET_REPLACEMENT) \
        return tp.process(replacements); \
    })()

#define MIN_SSR_MUSTACHE_WITH_DEFAULT(template_string, rules, unknown_handler) \
    ([&](){ \
        static constexpr auto tags = (MinSSR::sort_tags(MinSSR::Tags< \
            0 rules(FIY_MIN_SSR_TAGS_LEN)>({rules(FIY_MIN_SSR_TAG_CSV)})));\
        static const auto tp = MinSSR::ParsedTemplate::parse_mustache(template_string, tags, false);\
        MinSSR::Tags<tags.size()+1> replacements; \
        rules(FIY_MIN_SSR_SET_REPLACEMENT); \
        replacements[tags.size()] = unknown_handler; \
        return tp.process(replacements); \
    })()

#define MIN_SSR_MUSTACHE_VARIABLE_TEMPLATE(template_string, rules) \
    ([&](){ \
        static constexpr auto tags = (MinSSR::sort_tags(MinSSR::Tags< \
            0 rules(FIY_MIN_SSR_TAGS_LEN)>({rules(FIY_MIN_SSR_TAG_CSV)})));\
        MinSSR::Tags<tags.size()> replacements; \
        rules(FIY_MIN_SSR_SET_REPLACEMENT) \
        return MinSSR::mustache( template_string, tags, replacements); \
    })()

#define MIN_SSR_MUSTACHE_VARIABLE_TEMPLATE_WITH_DEFAULT(template_string, rules, unknown_handler) \
    ([&](){ \
        static constexpr auto tags = (MinSSR::sort_tags(MinSSR::Tags< \
            0 rules(FIY_MIN_SSR_TAGS_LEN)>({rules(FIY_MIN_SSR_TAG_CSV)})));\
        MinSSR::Tags<tags.size()+1> replacements; \
        rules(FIY_MIN_SSR_SET_REPLACEMENT); \
        replacements[tags.size()] = unknown_handler; \
        return MinSSR::mustache( template_string, tags, replacements, true); \
    })()
