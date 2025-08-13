//
// Created by tate on 7/30/25.
//

#include <ctime>

#include "../../../modlib/fediymod.hpp"

#include "Contact.hpp"

extern fiy::HostInfo g_host_info;

inline std::string get_timestamp_str(std::time_t t) {
    char buf[sizeof "yyyymmddThhmmssZ"];
    strftime(buf, sizeof buf, "%Y%m%dT%H%M%SZ", gmtime(&t));
    return buf;
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
        ret += "X-SOCIAL-PROFILE;TYPE=fediy:";
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

        ret += item.name;
        if (!item.params.empty()) {
            ret += ';';
            ret += item.params;
        }
        ret += ':';
        ret += item.value;
        ret += "\r\n";
    }

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

bool VC::parse(const std::string& vc) {
    if (!vc.starts_with("BEGIN:VCARD"))
        return false;
    return false;
}