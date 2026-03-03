//
// Created by tate on 2/27/26.
//

#pragma once

#include <string>

#include "../../../modlib/fediymod.hpp"

/**
 * Function that routes requests to correct handlers
 * @param path request subpath
 * @param cb request callback
 * @param req request object
 */
using RequestRouter = bool (*)(
    std::string_view path,
    fiy::Callback cb,
    fiy::Request &req
);
