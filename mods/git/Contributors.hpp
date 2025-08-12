//
// Created by tate on 7/9/24.
//

#ifndef FEDIY_CONTRIBUTORS_HPP
#define FEDIY_CONTRIBUTORS_HPP

#include <string>
#include <unordered_map>
#include <memory>

class User;
struct FederatedUser {
    std::string domain;
    std::string username;
    std::shared_ptr<User> user;
};

std::unordered_map<std::string, FederatedUser> parse_contributors_txt(const std::string& path) {

}


#endif //FEDIY_CONTRIBUTORS_HPP

