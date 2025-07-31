//
// Created by tate on 7/30/25.
//

#include <nlohmann/json.hpp>

#include "Contact.hpp"


std::string Contact::to_vcard() {
    std::string ret = "BEGIN:VCARD\nVERSION:4.0\n";

    return ret;
}

std::string Contact::to_json() {
    nlohmann::json::array_t fields;
    for (auto& [k , v] : m_fields)
        fields.emplace_back(nlohmann::json::array({ k, v }));

    nlohmann::json ret = {
        { "name", m_name },
        { "name", m_fiy_user },
        { "fields", std::move(fields) },
    };

    return ret.dump();
}