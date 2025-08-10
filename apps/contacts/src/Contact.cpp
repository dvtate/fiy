//
// Created by tate on 7/30/25.
//

#include <nlohmann/json.hpp>

#include "Contact.hpp"

std::vector<std::pair<std::string, std::string>> vcard_fields{{}};

std::string Contact::vcard() {
    std::string ret = "BEGIN:VCARD\nVERSION:4.0\n";

    ret += "END:VCARD";
    return ret;
}

std::string Contact::json() {
    nlohmann::json::array_t fields;
    for (auto& [k, v] : m_fields)
        fields.emplace_back(nlohmann::json::array({k, v}));

    nlohmann::json ret = {
        {"name", m_name},
        {"name", m_fiy_user},
        {"fields", std::move(fields)},
    };

    return ret.dump();
}

Contact Contact::json(const std::string& json_str) {
    // TODO
    return {};
}

Contact Contact::vcard(const std::string& vcard_str) {
    // TODO
    return {};
}