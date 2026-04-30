//
// Created by tate on 4/30/26.
//

#pragma once

#include <string>
#include <string_view>

#include "../../../../third_party/json/single_include/nlohmann/json.hpp"

// TODO instead do this?: https://github.com/nlohmann/json#basic-usage

/// Generic interface for encoding DTOs for communication between peers
struct DataTransferObject {
    DataTransferObject() = default;
    virtual ~DataTransferObject() = default;

    /// Convert to a JSON representation for communication between peers
    virtual nlohmann::json to_json() = 0;

    /// Update values
    virtual bool from_json(const nlohmann::json& json) = 0;

    bool json(const std::string_view json) {
        try {
            return from_json(nlohmann::json::parse(json));
        } catch (nlohmann::json::exception& e) {
            DEBUG_LOG("JSON error: " << e.what());
            return false;
        }
    }

    std::string json() {
        try {
            return to_json().dump();
        } catch (nlohmann::json::exception& e) {
            DEBUG_LOG("JSON error: " << e.what());
            throw e;
        }
    }
};
