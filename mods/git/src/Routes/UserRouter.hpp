//
// Created by tate on 4/7/26.
//

#pragma once

#include "Router.hpp"

bool user_router(
    std::string_view path,
    fiy::Callback cb,
    fiy::Request& req
);