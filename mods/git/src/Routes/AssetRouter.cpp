//
// Created by tate on 3/3/26.
//

#include "AssetRouter.hpp"

/**
 * Respond to requests for static assets
 * @param path url subpath
 * @param cb response callback
 * @param req request object
 * @return true if routed, false otherwise
 */
bool static_asset_router(
    std::string_view path,
    fiy::Callback cb,
    fiy::Request &req
) {

    return false;
}