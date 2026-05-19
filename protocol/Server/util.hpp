//
// Created by tate on 7/28/25.
//

#pragma once

#include <string>
#include <string_view>

#include "../defs.hpp"

template<class BoostHttpMessage>
inline std::string get_headers_string(const BoostHttpMessage& res) {
    // TODO there has to be a better way to do this ...
    std::string ret;
    for (const auto& f : res.base()) {
        ret += f.name_string();
        ret += ": ";
        ret += f.value();
        ret += '\n';
    }
    return ret;
}

/**
 * Set the headers for a response given a headers string
 * @tparam ResponseType can be any boost beast http message (even requests)
 * @param res response to set headers on
 * @param headers_str headers string to parse
 * @param allow_cors should the response include "access-control-allow-origin: *" header
 *      this is required for subdomain mods to work. This also adds "Vary: Origin"
 */
template<class ResponseType>
void response_set_headers(ResponseType& res, const char* headers_str, const bool allow_cors = false) {
    if (headers_str == nullptr)
        return;

    // This is required for subdomain mods
    if (allow_cors) {
        res.set(boost::beast::http::field::access_control_allow_origin, "*");
        res.set(boost::beast::http::field::vary, "Origin");
    }

    std::string_view headers{headers_str};
    while (!headers.empty()) {
        // Get field
        auto end = headers.find('\n');

        // Find colon
        auto colon = headers.find(':');
        if (colon >= end) {
            DEBUG_LOG("Invalid http header: " << headers.substr(0, end));
            return; // invalid
        }

        // Get header components
        auto name = headers.substr(0, colon);
        auto start = colon + 1;
        while ((headers[start] == 0x20 || headers[start] == 0x09) && start < end)
            start++;
        auto value = headers.substr(start, end - start);

        res.set(name, value);

        // Next if any
        if (end == std::string_view::npos)
            break;
        headers.remove_prefix(end + 1);
    }
}
