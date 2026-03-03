//
// Created by tate on 3/3/26.
//

#pragma once

#include "Router.hpp"

bool static_asset_router(
    std::string_view path,
    fiy::Callback cb,
    fiy::Request &req
);