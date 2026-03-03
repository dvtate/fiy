//
// Created by tate on 8/8/25.
//

#include "fediymod.h"
#include "fediymod.hpp"

namespace fiy {

    const char* fiy_http_verb_strings[] = {
        "<unknown>",
        "DELETE",
        "GET",
        "HEAD",
        "POST",
        "PUT",
        "CONNECT",
        "OPTIONS",
        "TRACE",
        "COPY",
        "LOCK",
        "MKCOL",
        "MOVE",
        "PROPFIND",
        "PROPPATCH",
        "SEARCH",
        "UNLOCK",
        "BIND",
        "REBIND",
        "UNBIND",
        "ACL",
        "REPORT",
        "MKACTIVITY",
        "CHECKOUT",
        "MERGE",
        "M-SEARCH",
        "NOTIFY",
        "SUBSCRIBE",
        "UNSUBSCRIBE",
        "PATCH",
        "PURGE",
        "MKCALENDAR",
        "LINK",
        "UNLINK"
    };

    // By default, everything is null
    // Host Host::info{ fiy_host_info_t{
    //     .domain = nullptr,
    //     .app_id = nullptr,
    //     .base_uri = nullptr,
    //     .data_dir = nullptr,
    //     .log = nullptr,
    //     .request = nullptr,
    //     .local_login = nullptr,
    //     .user_info = nullptr,
    //     .now = nullptr,
    //     .mod_config = nullptr
    // } };
}