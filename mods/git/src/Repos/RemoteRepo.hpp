//
// Created by tate on 3/2/26.
//

#pragma once

#include "BasicRepo.hpp"

/// Repo that resides on another instance
/// For now everything should get proxied to other server
/// but maybe in the future we can do some optimizations
class RemoteRepo : public BasicRepo {

};