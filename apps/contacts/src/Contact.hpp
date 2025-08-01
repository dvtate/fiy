//
// Created by tate on 7/30/25.
//

#pragma once

#include <string>
#include <utility>
#include <vector>

struct Contact {
    /// Unique ID
    int64_t m_id;

    /// Full name
    std::string m_name;

    /// Relevant fediy user
    std::string m_fiy_user;

    /// Relevant fields
    std::vector<std::pair<std::string, std::string>> m_fields;

//    static constexpr std::vector<std::pair<std::string, std::string>> FieldOptions;

    std::string vcard();

    std::string json();

    template<class IterableContainer>
    static std::string json_list(const IterableContainer& contacts) {
        std::string ret = "[";
        for (Contact c : contacts) {
            ret += c.json();
            ret += ',';
        }
        ret[ret.size() - 1] = ']';
        return ret;
    }
};
