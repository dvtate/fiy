
#include <mutex>
#include <string>
#include <memory>
#include <vector>

#include "GitRepo.hpp"

class User;

class Repo {
    // Which server is hosting the repo
    std::string m_domain;

    // Which user owns the repo
    std::string m_owner;

    // What is the name of the repo
    std::string m_reponame;

    bool m_private;
    std::vector<std::shared_ptr<User>> m_collaborators;
};

// How are we gonna show users?? emails no longer necessarily globally unique...
