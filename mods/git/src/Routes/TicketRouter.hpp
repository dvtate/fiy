//
// Created by tate on 4/12/26.
//

#pragma once

#include "Router.hpp"

// Called by repo router
// For paths like /user/repo/ticket/:number

bool ticket_router(
    std::string_view path,
    fiy::Callback cb,
    fiy::Request& req,
    const BasicRepo& repo
) {

}