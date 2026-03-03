//
// Created by tate on 12/8/25.
//

#pragma once

#include <vector>
#include <string>
#include <string_view>

#include "GitRepo.hpp"
#include "../../../../modlib/fediymod.hpp"

#include "LocalRepo.hpp"

struct Repos {
    std::unordered_map<std::string, LocalRepo*> m_repos;
};