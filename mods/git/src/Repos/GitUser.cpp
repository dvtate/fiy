//
// Created by tate on 4/30/26.
//

#include "GitUser.hpp"

#include <format>

/**
 * @return HTML for a link to user's profile or at least a titled span
 */
std::string GitUser::profile_link() const {
    if (!fiy_user.empty()) {
        return std::format("<a href=\"{}/{}\">{}</a>",
            fiy::host().base_uri,
            fiy_user,
            fiy_user);
        // ret = "<a href=\"";
        // ret += fiy::host().base_uri;
        // ret += '/';
        // ret += fiy_user;
        // ret += "\">";
        // ret += fiy_user;
        // ret += "</a>";
        // return ret;
    }

    std::string ret;
    ret = "<span title=\"";
    if (name.empty()) {
        ret += email.empty() ? "unknown" : email;
    } else {
        ret += name;
        if (!email.empty()) {
            ret += " <";
            ret += email;
            ret += ">";
        }
    }
    ret += "\">";
    ret += name.empty()
        ? (email.empty() ? "<unknown>" : email)
        : name;
    ret += "</span>";
    return ret;
}
