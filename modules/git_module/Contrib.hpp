#pragma once

#include "Repo.hpp"
#include "User.hpp"

class Contrib {
public:
    std::shared_ptr<User> m_user;
    std::shared_ptr<Repo> m_repo;
    size_t m_ts;
};