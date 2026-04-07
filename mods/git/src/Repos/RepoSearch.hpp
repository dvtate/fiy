//
// Created by tate on 4/3/26.
//

#pragma once

#include <string>
#include <vector>

#include "BasicRepo.hpp"

struct RepoListEntry {
    std::string instance;
    std::string owner;
    std::string name;
    std::string description;
    unsigned int likes;
    bool is_fork;
};

struct RepoSearch {
    /// Max number of results to return
    static constexpr int MAX_RESULTS = 1000;

    std::string owner;
    std::string name_like;
    std::string description_like;
    std::string query;

    int32_t min_likes{-1};
    uint16_t limit{20};
    uint16_t page{0};

    enum Sort {
        Age,
        Likes,
        Name,
        EditDate,
    };

    Sort sort : 3 {Name};
    bool order_desc: 1 { true};

    enum BooleanFilter {
        Any,    // No filter
        Yes,    // Only include yes
        No,     // Only include no
    };
    BooleanFilter forks: 2 {Any};

    // Visibility
    fiy::Locality visibility : 3;
    bool filter_visibility : 1 {false};

    void set_min_likes(const int new_min_likes) {
        this->min_likes = new_min_likes;
    }
    bool set_limit(const int new_limit) {
        if (new_limit > MAX_RESULTS) {
            this->limit = MAX_RESULTS;
            return false;
        }

        this->limit = new_limit;
        return true;
    }
    void set_page(const int new_page) {
        this->page = new_page;
    }
    void set_sort(const Sort new_sort) {
        this->sort = new_sort;
    }
    void set_owner(const std::string& new_owner) {
        this->owner = new_owner;
    }
    void include_forks(const BooleanFilter forks_filter) {
        this->forks = forks_filter;
    }
    void set_any_visibility() {
        this->filter_visibility = false;
    }
    void set_visibility_filter(const fiy::Locality filter) {
        this->visibility = filter;
        this->filter_visibility = true;
    }
    void set_name_like(const std::string& name) {
        this->name_like.reserve(name.size() + 2);
        this->name_like = "%" + name + '%';
    }
    void set_description_like(const std::string& description) {
        this->description_like.reserve(description.size() + 2);
        this->description_like = "%" + description + '%';
    }

    std::vector<BasicRepo> search(const std::string& requesting_user = "", const std::string& requesting_user_instance = "");

    // TODO need a way to specify what to include in search results (ie - description, likes, etc.)
};
