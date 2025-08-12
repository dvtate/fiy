//
// Created by tate on 7/28/25.
//

#ifndef FEDIY_UTIL_HPP
#define FEDIY_UTIL_HPP


#include <string>
#include <string_view>

#include "defs.hpp"

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

template<class ResponseType>
void response_set_headers(ResponseType& res, const char* headers_str) {
    if (headers_str == nullptr)
        return;

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

#endif //FEDIY_UTIL_HPP
