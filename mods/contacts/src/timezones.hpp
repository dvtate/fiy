//
// Created by tate on 9/23/25.
//

#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "../../../third_party/json/single_include/nlohmann/json.hpp"

/**
 * Get timezone database entries from the system
 * @remark the result should be cached
 * @return timezone database object
 *  {   version: string,
 *      zones: [offset number, abbreviation string][],
 *      links: [name string, target string][] }
 */
inline std::string get_timezones_json() {
    // Get tz database
    const auto& db = std::chrono::get_tzdb();

    // Get zone info
    const auto now = std::chrono::system_clock::now();
    auto zones = nlohmann::json::object();
    for (const auto& z : db.zones) {
        const auto info = z.get_info(now);
        zones.emplace(z.name(), nlohmann::json::array({
            info.offset.count(),
            info.abbrev
        }));
    }

    // Get zone aliases
    std::vector<nlohmann::json> links;
    links.reserve(db.links.size());
    for (const auto& l : db.links)
        links.emplace_back(nlohmann::json::array({ l.name(), l.target() }));

    // Return JSON
    const nlohmann::json ret = {
        { "version", db.version },
        { "zones", std::move(zones) },
        { "links", std::move(links) },
    };
    return ret.dump();
}