//
// Created by tate on 7/9/24.
//

#include <cstring>
#include <string>

#include "../../modlib/fediymod.h"

/*
/// MVP Endpoints

////
// Pages - these give html
////
GET /<user>
GET /<user>/settings
POST /<user>/settings
GET /new
GET /<user>/<repo>
GET /<user>/<repo>/.git

////
// Api Endpoints - these give JSON, called by other servers and apps
///
// Users
GET /api/users/<user>

// Repos
GET /api/repos/<user>/<repo>
*/

static void handle_request(fediy::fiy_request_t* request, fediy::fiy_callback_t callback) {
    fediy::fiy_response_t resp;

    std::string_view path = request->path;
    if (path == )

    path.remove_prefix(1); // remove leading /
    auto slash_pos = path.find('/');
    std::string_view component = path.substr(0, slash_pos);

    if (component == "api") {
        // handle api endpoints
        goto respond;
    } else if (component == "new") {
        // new repo endpoint
        goto respond;
    } else if (
        component == "issues"
        || component == "notifications"
        || component == ""
    ) {
        goto respond;
    }

    resp = {
        .status=404,
        .body="not found"
    };

respond:
    callback(&resp);
}

