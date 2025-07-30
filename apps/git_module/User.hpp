#pragma once

#include <string>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <variant>

#include "Repo.hpp"
#include "Contrib.hpp"

class Org;

class User {
protected:
    // cache
    std::mutex m_mtx;
    std::deque<std::shared_ptr<Repo>> m_repos;
    std::deque<std::shared_ptr<Contrib>> m_contribs;
    std::vector<std::shared_ptr<User>> m_following;
    std::vector<std::shared_ptr<User>> m_followers;
    std::vector<std::shared_ptr<Org>> m_orgs;

public:
    std::string m_domain;
    std::string m_username;

    std::string m_name;
    std::string m_about;
    std::vector<std::string> m_links;
};

class Org {
protected:
    // cache
    std::mutex m_cache_lock;
    std::deque<std::shared_ptr<Repo>> m_repos_cache;
    std::deque<std::shared_ptr<User>> m_members_cache;

public:
    std::string m_domain;
    std::string m_username;
    std::string m_name;
    std::string m_about;

};

class UserCache {
    std::mutex m_mtx;

    // domain -> username -> user |
    std::unordered_map<std::string,
        std::unordered_map<std::string,
            std::variant<Org, User>>> m_cache;


};