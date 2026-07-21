//
// Created by tate on 5/28/26.
//
#include "Pages.hpp"

#include "../../../util/MinSSR.hpp"


std::string Pages::user_page(const std::string& user, const char* request_user) {
    static constexpr char user_page[] = "/user.html";
    const auto& template_string = file_contents<user_page>();

    #define USER_PAGE_RULES(KV) \
        KV("fiy_user", user) \
        KV("fiy_domain", fiy::host().domain) \
        KV("request_user", request_user == nullptr ? "" : request_user) \
        KV("mod_baseurl", std::string_view(fiy::host().base_uri)) \
        KV("fiy_display_name", user) \
        KV("fiy_user_bio", "")

    return MIN_SSR_MUSTACHE(template_string, USER_PAGE_RULES);
}
