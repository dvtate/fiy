//
// Created by tate on 4/30/26.
//

#include "Router.hpp"

#include "ApiRouter.hpp"
#include "Pages.hpp"
#include "AssetRouter.hpp"
#include "RepoRouter.hpp"
#include "UserRouter.hpp"

/**
 * Request handler
 * @param request incoming user request from host
 * @param cb callback from host
 */
void handle_request(struct fiy::fiy_request_t* request, fiy::Callback cb) {
    auto& req = *(fiy::Request*)request;

    std::string_view path = req.path;

    if ( static_asset_router(path, cb, req)
        || user_router(path, cb, req)
        || api_router(path, cb, req)
        || repo_router(path, cb, req)
    )
        return;

    // No router
    // TODO custom 404 page?
    req.respond(cb, 404, "", "Not Found");
}
