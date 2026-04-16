//
// Created by tate on 4/12/26.
//

#pragma once

#include "Router.hpp"

bool ticket_router(
    std::string_view path,
    fiy::Callback cb,
    fiy::Request& req
);