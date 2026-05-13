//
// Created by tate on 4/3/26.
//

#pragma once

#include <string>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

#include <nlohmann/json.hpp>

#include "../DB.hpp"

#include "BasicRepo.hpp"


struct RepoSearch {
    // TODO separate structs for filters and results

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

    /// Visibility
    fiy::Locality visibility : 3;
    bool filter_visibility : 1 {false};

    /// Should we include pagination data in results?
    bool page_data : 1 { false };

    /// Output data components
    enum class Parts : uint8_t {
        Invalid = 0,        // this field is invalid
        InternalId,
        Path,
        Description,
        Likes,
        Visibility,
        Fork,
        CreateTs,
        UpdateTs,
        Tickets,
    };

    static const std::vector<Parts> default_fields;

    /// Fields what fields should we include in results
    std::vector<Parts> fields;

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
    void set_owner(const std::string_view new_owner) {
        if (auto at = new_owner.find_first_of("@%");
            at != std::string_view::npos
            && new_owner.substr(at + 1) == fiy::host().domain
        )
            this->owner = new_owner.substr(0, at);
        else
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

    void set_fields(std::string_view csv);

    // std::vector<BasicRepo> search(
    //     const std::string& requesting_user = "",
    //     const std::string& requesting_user_instance = ""
    // );

    nlohmann::json search(
        const std::string& requesting_user = "",
        const std::string& requesting_user_instance = ""
    );


protected:
    std::string fields_select_part();
    nlohmann::json row_json(const SQLite::Statement& stmt) const;
};
