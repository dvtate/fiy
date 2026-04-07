//
// Created by tate on 7/30/25.
//

#include <ctime>
#include <regex>

#include <nlohmann/json.hpp>

#include "../../../modlib/fiymod.hpp"

#include "Contact.hpp"

inline std::string get_timestamp_str(const std::time_t t) {
    char buf[sizeof "yyyymmddThhmmssZ"];
    strftime(buf, sizeof buf, "%Y%m%dT%H%M%SZ", gmtime(&t));
    return buf;
}

inline time_t parse_timestamp_str(const std::string& str) {
    // Parse timestamp
    // The server and the date are both expected to be in UTC
    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    sscanf(str.c_str(), "%4d%2d%2dT%2d%2d%2d", &year, &month, &day, &hour, &min, &sec);
    std::tm t{};
    t.tm_sec = sec;
    t.tm_min = min;
    t.tm_hour = hour;
    t.tm_mday = day;
    t.tm_mon = month - 1;
    t.tm_year = year - 1900;
    return std::mktime(&t);
}

std::string VC::to_vcard() const {
    if (invalid())
        return "";

    std::string ret = "BEGIN:VCARD\r\nVERSION:4.0\r\n";

    if (this->id >= 0) {
        // Add SOURCE field
        ret += "SOURCE:";
        ret += fiy::host().base_uri;
        ret += "/id/";
        ret += std::to_string(this->id);
        ret += "\r\n";

        // Add UID field
        ret += "UID:";
        ret += std::to_string(this->id);
        ret += "\r\n";
    }
    if (!this->user.empty() && this->owner == this->user) {
        ret += "X-FIY-PROFILE:";
        ret += this->user;
        ret += '@';
        ret += fiy::host().domain;
        ret += "\r\n";
    }
    if (!this->user.empty()) {
        ret += "X-SOCIALPROFILE;TYPE=fiy:";
        ret += this->user;
        ret += '@';
        ret += fiy::host().domain;
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


// TODO accept string_view
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

        // Set by us
        if (name == "VERSION")
            continue; // All are expected to version 4
        if (name == "UID") {
            try {
                this->id = std::stoll(value);
            } catch (...) {
                std::string msg = "VC::parse(): Invalid UID: " + value;
                fiy::log_warning(msg);
            }
            continue;
        }
        if (name == "X-FIY-PROFILE") {
            const auto [u, domain] = fiy::host().split_user_str(value);
            if (domain.empty())
                this->user = u;
            else
                this->user = value;
            // if (this->owner != value) {
            //     fiy::log_error("VC::parse(): User not owner of profile card? "
            //         + this->user + " != " + this->owner);
            // }
            continue;
        }

        // We store this differently
        if (name == "REV") {
            this->update_ts = parse_timestamp_str(value);
            continue;
        }

        if (name == "SOURCE") {
            continue; // we are the source now
            // if (value.find(fiy::host().domain) != std::string::npos) {
            //     if (this->id == -1)
            //         continue;
            //     if (value.ends_with("/id/" + std::to_string(this->id)))
            //         continue;
            // }
        }

        // Handle parameters
        const std::string raw_params = line.substr(first_semi_idx, colon_idx - first_semi_idx);
        int8_t visibility = 0;
        int64_t property_id = -1;
        std::string params;
        if (!raw_params.empty()) {
            for (auto&& p : resplit(raw_params, std::regex(";"))) {
                if (p.empty())
                    continue;
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
                    if (p.starts_with("TYPE=fiy") && name == "X-SOCIALPROFILE") {
                        const auto [u, domain] = fiy::host().split_user_str(value);
                        if (domain.empty())
                            this->user = u;
                        else
                            this->user = value;
                        goto skip_property; // alt for X-FIY-PROFILE
                    }

                    // TODO check if no = ?
                    params += ';';
                    params += p;
                }
            }
            if (params[0] == ';')
                params.erase(0, 1);
            if (params[0] == ';')
                params.erase(0, 1);
        }

        // Add property
        this->props.emplace_back(Prop{
            .id=property_id,
            .name=name,
            .params=params,
            .value=value,
            .visibility=visibility
        });
skip_property:;
    }

    return true;
}

VC VC::basic_profile(const std::string& user) {
    VC ret;
    ret.user = user;
    ret.update_ts = fiy::host().now();
    return ret;
}

static std::string convert_structured_name(const std::string_view value) {
    /* js
    const [
        surnames,
        givenNames,
        additionalNames,
        prefixes,
        suffixes
    ] = this.value
        .split(';')
        .map(n => n.split(','));
    return [(prefixes?.join(' ') || ''),
        (givenNames?.join(' ') || ''),
        (additionalNames?.join(' ') || ''),
        (surnames?.join(' ') || ''),
        (suffixes?.join(' ') || '')
    ].filter(n => !!n).join(' ');
    */

    std::string_view::size_type i = 0;

    // Pull off a semicolon delineated part of the input string_view
    auto get_part = [&]() {
        if (i >= value.size())
            return std::string_view{};
        const auto end = value.find(';', i);
        const auto ret = value.substr(i, end);
        i = end;
        if (i != std::string_view::npos)
            ++i;
        return ret;
    };

    std::string_view surnames = get_part();
    std::string_view given_names = get_part();
    std::string_view additional_names = get_part();
    std::string_view prefixes = get_part();
    std::string_view suffixes = get_part();

    std::string ret;

    // Calculate size of return string
    size_t total_size = 0;
    uint8_t count = 0;
    if (surnames.size()) {
        total_size += surnames.size();
        count++;
    }
    if (given_names.size()) {
        total_size += given_names.size();
        count++;
    }
    if (additional_names.size()) {
        total_size += additional_names.size();
        count++;
    }
    if (prefixes.size()) {
        total_size += prefixes.size();
        count++;
    }
    if (suffixes.size()) {
        total_size += suffixes.size();
        count++;
    }
    // spaces between name parts
    if (count > 1)
        total_size += count - 1;
    ret.reserve(total_size);

    // Convert commas into spaces
    bool needs_space = false;
    auto append_part = [&](const std::string_view value) {
        if (value.empty())
            return;

        if (needs_space) {
            ret += ' ';
            // needs_space = false;
        } else {
            needs_space = true;
        }

        for (const char c : value)
            if (c == ',')
                ret += ' ';
            else
                ret += c;
    };

    append_part(prefixes);
    append_part(given_names);
    append_part(additional_names);
    append_part(surnames);
    append_part(suffixes);
    return ret;
}

/**
 * JSON representation of the card
 * @return JSON of form: {
 *    name: User display name (FN/N),
 *    nick: Nickname (NICK),
 *    email: user email,
 *    bio: NOTES,
 *    socials: list of X-SOCIAL-PROFILE values,
 *    websites: list of WEBSITE values,
 * }
 */
std::string VC::to_internal_json() const {
    std::string name, nickname, email, bio;
    std::vector<std::string> websites;
    std::vector<std::string> socials;

    auto n_field = static_cast<size_t>(-1);
    for (size_t i = 0; i < this->props.size(); i++) {
        const auto& p = this->props[i];
        if (p.name == "FN") {
            if (name.empty())
                name = p.value;
        } else if (p.name == "EMAIL") {
            if (email.empty())
                email = p.value;
        } else if (p.name == "NICKNAME") {
            if (nickname.empty())
                nickname = p.value;
        } else if (p.name == "NOTE") {
            if (bio.empty())
                bio = p.value;
        } else if (p.name == "URL")
            websites.emplace_back(p.value);
        else if (p.name == "X-SOCIALPROFILE")
            socials.emplace_back(p.value);
        else if (p.name == "N")
            if (n_field == static_cast<size_t>(-1))
                n_field = i;
    }

    // If no FN field, look for N field
    if (n_field != static_cast<size_t>(-1) && name.empty())
        for (const Prop& p : this->props)
            if (p.name == "N") {
                name = convert_structured_name(p.value);
                break;
            }

    // Convert to JSON
    auto ret = nlohmann::json::object();
    ret["name"] = std::move(name);
    ret["nick"] = std::move(nickname);
    ret["email"] = std::move(email);
    ret["bio"] = std::move(bio);
    ret["socials"] = std::move(socials);
    ret["websites"] = std::move(websites);
    return ret.dump();
}
