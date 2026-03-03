//
// Created by tate on 2/27/26.
//

#include <nlohmann/json.hpp>

#include "../../modlib/fediymod.hpp"

std::string g_proxy_target;

void handle_request(fiy::Request& r, const fiy::Callback cb) {
    // TODO the hard part: proxy request with added user header
}

/// Export: Start module
extern "C" fiy::ModInfo* start(const fiy_host_info_t* host_info) {
    // Read config
    fiy::Host::set(*host_info);
    auto config = nlohmann::json::parse(fiy::Host::info.mod_config);
    if (!(config.contains("mod_settings") && config.is_object()
        && (config = config["mod_settings"]).contains("target"))
    ) {
        fiy::log_fatal("missing required field: mod_settings.target: should be url to proxy");
        return nullptr;
    }
    if (!config["target"].is_string()) {
        fiy::log_fatal("mod_settings.target should be string containing url to proxy");
        return nullptr;
    }
    g_proxy_target = config["target"].get<std::string>();

    // Exchange info with host
    static fiy::ModInfo mod_info = {
        .on_request = [](fiy_request_t* r, const fiy::fiy_callback_t cb) {
            handle_request(static_cast<fiy::Request&>(*r), cb);
        },
        .delete_user = nullptr
    };
    return &mod_info;
}