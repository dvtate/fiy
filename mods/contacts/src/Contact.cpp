//
// Created by tate on 7/30/25.
//

#include <ctime>
#include <regex>

#include "../../../modlib/fediymod.hpp"

#include "Contact.hpp"

extern fiy::HostInfo g_host_info;

inline std::string get_timestamp_str(std::time_t t) {
    char buf[sizeof "yyyymmddThhmmssZ"];
    strftime(buf, sizeof buf, "%Y%m%dT%H%M%SZ", gmtime(&t));
    return buf;
}

inline time_t parse_timestamp_str(const std::string& str) {
    // Parse timestamp
    // The server and the date are both expected to be in UTC
    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    sscanf(str.c_str(), "%4d%2d%2dT%2d%2d%2d", &year, &month, &day, &hour, &min, &sec);
    std::tm t;
    t.tm_sec = sec;
    t.tm_min = min;
    t.tm_hour = hour;
    t.tm_mday = day;
    t.tm_mon = month - 1;
    t.tm_year = year - 1900;
    return std::mktime(&t);
}

std::string VC::to_vcard() {
    std::string ret = "BEGIN:VCARD\r\nVERSION:4.0\r\n";

    if (this->id >= 0) {
        ret += "SOURCE:";
        ret += g_host_info.base_uri;
        ret += "/id/";
        ret += std::to_string(this->id);
        ret += "\r\n";
    }
    if (!this->user.empty() && this->owner == this->user) {
        ret += "X-FEDIY-PROFILE:";
        ret += this->user;
        ret += "\r\n";
    }
    if (!this->user.empty()) {
        ret += "X-SOCIALPROFILE;TYPE=fediy:";
        ret += this->user;
        ret += "@";
        ret += g_host_info.domain;
        ret += "\r\n";
    }
    if (this->update_ts) {
        ret += "REV:";
        ret += get_timestamp_str(this->update_ts);
        ret += "\r\n";
    }

    bool has_name = false;
    for (const auto& item: this->props) {
        if (item.name == "N" || item.name == "FN")
            has_name = true;

        // Property name
        ret += item.name;

        // Params
        if (!item.params.empty()) {
            ret += ';';
            ret += item.params;
        }
        if (item.visibility != 0 || this->user == this->owner) {
            ret += ';';
            ret += "X-FIY-VISIBILITY=";
            ret += '0' + item.visibility;
        }
        if (item.id >= 0) {
            ret += ';';
            ret += "X-FIY-ID=";
            ret += std::to_string(item.id);
        }

        // Property value
        ret += ':';
        ret += item.value;
        ret += "\r\n";
    }

    // Add name if missing
    if (!has_name) {
        if (!this->user.empty()) {
            ret += "FN:";
            ret += this->user;
            ret += "\r\n";
        } else {
            ret += "FN:\r\n";
        }
    }

    ret += "END:VCARD";

    return ret;
}

std::vector<std::string> resplit(const std::string &s, const std::regex &sep_regex = std::regex{"\\s+"}) {
    std::sregex_token_iterator iter(s.begin(), s.end(), sep_regex, -1);
    std::sregex_token_iterator end;
    return {iter, end};
}



bool VC::parse(std::string vc) {
    // Trim string
    vc.erase(0, vc.find_first_not_of(" \t\n\r"));
    vc.erase(vc.find_last_not_of(" \t\n\r") + 1);

    // Not a vcard
    if (!vc.starts_with("BEGIN:VCARD"))
        return false;

    std::string input = "apple banana apple orange apple";
    std::regex pattern("apple");
    std::string replacement = "grape";

    // Replace escaped newlines
    vc = std::regex_replace(vc, std::regex("\\n\\s"), "");

    // Parse properties
    std::regex line_regex{R"(\r\n(?=\S)|\r(?=\S)|\n(?=\S))"};
    for (std::sregex_token_iterator it(vc.begin(), vc.end(), line_regex, -1), end; it != end; ++it) {
        std::string line {*it};

        const auto colon_idx = line.find(':');
        if (colon_idx == std::string::npos)
            return false;
        const auto value = line.substr(colon_idx+1);

        size_t first_semi_idx = 0;
        for (; first_semi_idx < colon_idx; first_semi_idx++)
            if (line[first_semi_idx] == ';')
                break;
        const std::string name = line.substr(0, first_semi_idx);

        // nonsense
        if (name == "BEGIN" || name == "END")
            continue;

        // Obsolete
        if (name == "AGENT" || name == "CLASS" || name == "MAILER" || name == "PROFILE" || name == "SORT-STRING")
            continue;

        // Not supported
        if (name == "CLIENTPIDMAP")
            continue;

        // These are set by us
        if (name == "VERSION")
            continue; // All are expected to version 4
        if (name == "UID")
            continue; // we set this

        // We store this differently
        if (name == "REV") {
            this->update_ts = parse_timestamp_str(value);
            continue;
        }

        // Handle parameters
        const std::string raw_params = line.substr(first_semi_idx, colon_idx - first_semi_idx);
        int8_t visibility = 0;
        int64_t property_id = -1;
        std::string params;
        if (!raw_params.empty())
            for (auto&& p : resplit(raw_params, std::regex(";"))) {
                if (p.starts_with("X-FIY-VISIBILITY=")) {
                    visibility = p[17] - '0';
                    if (visibility > 3)
                        visibility = 3;
                } else if (p.starts_with("X-FIY-ID=")) {
                    char* pend;
                    const char* idstr = p.c_str() + 9;
                    property_id = strtol(idstr, &pend, 10);
                    if (pend == idstr)
                        property_id = -1;
                } else {
                    if (p.starts_with("TYPE=fediy") && name == "X-SOCIALPROFILE")
                        this->user = value;

                    // TODO check if no = and make it to TYPE=lhs
                    params += ';';
                    params += p;
                }
            }
        if (params[0] == ';')
            params = params.substr(1);

        // Add property
        this->props.emplace_back(Prop{
            .id=property_id,
            .name=name,
            .params=params,
            .value=value,
            .visibility=visibility
        });
    }

    return true;
}

VC VC::basic_profile(const std::string& user) {
    VC ret;
    ret.user = user;
    ret.update_ts = g_host_info.now();
    return ret;
}
