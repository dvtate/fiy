//
// Created by tate on 3/2/26.
//

#pragma once

#include "Router.hpp"

bool api_router(
    std::string_view path,
    fiy::Callback cb,
    fiy::Request& req
);