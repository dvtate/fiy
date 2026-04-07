//
// Created by tate on 2/27/26.
//

#pragma once

#include "Router.hpp"

bool repo_request_router(
    std::string_view path,
    fiy::Callback cb,
    fiy::Request& req
);