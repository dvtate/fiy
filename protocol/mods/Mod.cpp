#include "Mod.hpp"


#include <dlfcn.h>
#include <fstream>
#include <filesystem>

#include <boost/unordered/unordered_flat_set.hpp>

#include <nlohmann/json.hpp>

#include "../../util/FileCache.hpp"

#include "../FIY.hpp"

#include "ModConnectorDll.hpp"
#include "ModConnectorNet.hpp"

[[nodiscard]] inline std::filesystem::path Mod::dir() const {
    return g_fiy->config.data_dir + "/mods/" + data_dir;
}

Mod::Mod(const std::string& dir) {
    this->data_dir = dir;
    this->user_data_dir = dir;
    load();
}

Mod::~Mod() {
    stop();
}

bool Mod::start() {
    std::lock_guard lock(m_mtx);

    // Note: this function can update our fields if not populated from module.json
    return m_running = ipc->start();
}

bool Mod::stop() {
    std::lock_guard lock(m_mtx);
    m_running = false;
    return ipc->stop();
}

/**
 * @return module.json with relevant values updated
 */
std::string Mod::json() {
    // TODO include fields we don't use here
    auto json = parse_file();

    // Identifier specifying the protocol the mod implements
    // Multiple mods can have the same id only if they're compatible
    //      ie - chat.v3
    json["id"] = id;

    // This is the path that the users can use to access the mod (locally unique)
    json["path"] = path;

    // Mod version of the form X.y or just X
    json["version"] = version.str();

    // User-readable name for the mod
    json["name"] = name;

    // User-readable description of the mod and what it does
    json["description"] = description;

    // Mod icon
    json["icon"] = icon;

    // User can disable mods they don't want
    json["enabled"] = m_enabled;

    // How does this mod connect to the protocol server
    // json["connector"] = m_ipc->type() == ModConnector::Type::NETWORK
    //             ? m_ipc->m_uri.starts_with("https")
    //                 ? "https"
    //                 : "http"
    //             : m_ipc->type() == ModConnector::Type::SHARED_LIBRARY
    //                 ? "shared_object"
    //                 : "socket";

    json["connector_uri"] = ipc->uri();
    return json.dump();
}

/**
 * @return Mod description for frontend
 */
std::string Mod::user_json() {
    nlohmann::json json = {
        { "id", id },
        { "path", path },
        { "version", version.str() },
        { "name", name },
        { "description", description },
        { "icon", icon.empty()
            ? "/portal/app.svg"             // default app icon
            : "/mods/" + id + "/" + icon
        },
        { "status", status() }, // TODO give string instead
        { "loaded", m_loaded },
        { "enabled", m_enabled },
        { "running", m_running },
        { "error", m_error },
    };
    return json.dump();
}

void Mod::save() {
    // Mutex should be locked before this operation
    std::ofstream out(dir() / "module.json");
    out <<json();
    out.close();
}

/**
 *
 * @param value user-provided json value
 *  One of the following values
 *  - `"public"` (default): Grants access to anyone, even anonymous users
 *  - `"federated"` : Grants access to any authenticated user, even those on other instances
 *  - `"local"` : Grants access to users of this instance
 *  - `"user1,user2,user3"`: grants access to a specific list of users
 * @return access checker functor
 */
Mod::AccessChecker Mod::parse_access_checker(const std::string& value) {
    // Global access
    if (value.empty() || value == "public")
        return [](const char*, const char*) {
            return true;
        };

    // No access
    if (value == "disabled")
        return [](const char*, const char*) {
            return false;
        };

    // Federated users
    if (value == "federated")
        return [](const char* user, const char*) {
            return user != nullptr;
        };

    // Only Instance-local Users
    if (value == "local")
        return [](const char* user, const char* domain) {
            return user != nullptr && domain == nullptr;
        };

    // value contains comma-separated whitelisted usernames
    // Split by commas, ignoring leading/trailing whitespace and empty values
    boost::unordered_flat_set<std::string> whitelist;
    const char* start = value.c_str();
    while (*start == ' ' || *start == ',')
        start++;
    while (*start) {
        // Find end of value
        const char* end = start;
        while (*end && *end != ' ' && *end != ',')
            end++;

        // Store value
        whitelist.emplace(start, end);

        // Next iteration, skip whitespace and commas
        while (*end == ' ' || *end == ',')
            end++;
        start = end;
    }

    // Checker that checks whitelist for local users
    return [wl = std::move(whitelist)](const char* user, const char* domain) {
        return domain == nullptr && wl.contains(user);
    };
}

void Mod::load() {
    if (id.empty())
        id = data_dir;
    if (path.empty())
        path = id;

    // Set error and log
    m_error.clear();

    // Load JSON from file
    auto conf = parse_file();

    // Override id
    if (conf.contains("id")) {
        auto value = conf.at("id");
        if (!value.is_string()) {
            load_error("module.json: \"id\" should be a string");
        } else {
            auto new_id = value.get<std::string>();
            if (id != new_id)
                LOG_ERR(id << "/module.json: \"id\" field (" <<new_id
                    <<") does not match directory name, the mod may not work");
            id = new_id;
        }
    }

    // Get routing path
    if (conf.contains("path")) {
        auto value = conf.at("path");
        if (!value.is_string()) {
            load_error("module.json: \"path\" should contain a uri component string");
        } else {
            // TODO validate uri component
            this->path = value.get<std::string>();
        }
    }

    // Get name
    if (!conf.contains("name")) {
        load_error("module.json: missing key: name");
    } else {
        auto value = conf.at("name");
        if (!value.is_string()) {
            load_error("module.json: \"name\" should be a string");
        } else {
            this->name = value.get<std::string>();
        }
    }

    // Get version
    if (conf.contains("version")) {
        auto value = conf.at("version");
        if (!value.is_string()) {
            load_error("module.json: \"version\" should be a string of format NN.nn");
        } else {
            this->version = Version(value.get<std::string>());
        }
    } else {
        // load_error("module.json: missing key: version");
    }

    // Get description
    if (!conf.contains("description")) {
        load_error("module.json: missing key: description");
    } else {
        auto value = conf.at("description");
        if (!value.is_string()) {
            load_error("module.json: \"description\" should be a string");
        } else {
            this->description = value.get<std::string>();
        }
    }

    // Set up IPC (order matters)
    std::string connector_uri;
    if (conf.contains("connector_uri")) {
        auto value = conf.at("connector_uri");
        if (!value.is_string()) {
            load_error("module.json: \"connector_uri\" should be a string");
        } else {
            connector_uri = value.get<std::string>();
        }
    }
    if (!conf.contains("connector")) {
        load_error("module.json: missing key: connector");
    } else {
        auto value = conf.at("connector");
        std::string ts;
        if (!value.is_string()) {
            load_error("module.json: \"connector\" should be either \"shared_object\", \"socket\" or \"tcp\"");
        } else {
            ts = value.get<std::string>();
        }

        if (ts == "http" || ts == "https") {
            if (connector_uri.empty()) {
                load_error("module.json: \"connector_uri\" must be defined when \"ipc\" is set to \"" + ts + "\"");
            } else {
                this->ipc = std::make_unique<ModConnectorNet>(this, connector_uri, ts == "https");
            }
        } else if (ts == "shared_object") {
            if (connector_uri.empty())
                connector_uri = dir() / "module.so";
            this->ipc = std::make_unique<ModConnectorDll>(this, connector_uri);
        } else if (ts == "socket") {
            if (connector_uri.empty())
                connector_uri = dir() / "ipc.sock";
//            this->ipc = std::make_unique<ModSockIPC>(connector_uri);
        }
    }

    if (conf.contains("enabled")) {
        auto value = conf.at("enabled");
        if (value.is_boolean()) {
            m_enabled = value.get<bool>();
        } else if (value.is_number()) {
            m_enabled = value.get<double>() != 0;
        } else {
            load_error("module.json: \"enabled\" should be a boolean");
        }
    } else {
        m_enabled = false;
    }

    if (conf.contains("icon")) {
        auto value = conf.at("icon");
        if (value.is_string()) {
            this->icon = value.get<std::string>();
        } else if (value.is_null()) {
            // Use default icon
            this->icon = "";
        } else {
            load_error("module.json: \"icon\" should be a string containing"
                " a path to request the module to get the icon");
        }
    }

    if (conf.contains("access")) {
        auto value = conf.at("access");
        if (value.is_string()) [[likely]] {
            can_access = parse_access_checker(value.get<std::string>());
        } else if (value.is_boolean()) {
            can_access = parse_access_checker(value.get<bool>() ? "" : "disabled");
        } else {
            load_error("module.json: \"access\" should be either \"public\", \"federated\", \"local\","
                " or a comma-separated list of local users allowed to use the mod");
        }
    } else {
        can_access = parse_access_checker("");
    }

    if (conf.contains("data_dir")) {
        auto value = conf.at("data_dir");
        if (value.is_string()) [[likely]] {
            user_data_dir = value.get<std::string>();
        } else {
            load_error("module.json: \"data_dir\" should be a string");
        }
    } else {
        user_data_dir = dir();
    }

    // Get install ts
    try {
        install_ts = std::filesystem::last_write_time(dir());
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERR("Mod::load: failed to get fs::last_write_time" <<e.what());
    }

    if (m_error.empty())
        m_loaded = true;
}

void Mod::load_error(const std::string& message) {
    m_error += message;
    m_error += '\n';
    m_loaded = false;
    LOG_ERR(id << ": " <<message);
}

nlohmann::json Mod::parse_file() {
    const std::filesystem::path config_file = dir() / "module.json";
    if (!std::filesystem::exists(config_file)) {
        load_error("missing module.json");
        return{};
    }

    // Load config
    config = config_file.string();
    try {
        std::ifstream ifs(config_file);
        auto ret = nlohmann::json::parse(ifs);
        if (!ret.is_object()) {
            load_error("module.json: should be an object");
            return{};
        }
        return ret;
    } catch (const nlohmann::json::parse_error& e) {
        load_error(dir().string() + "/module.json: invalid JSON: " + e.what());
        return {};
    }
}

void Mod::set_enabled(const bool enabled) {
    std::lock_guard lock(m_mtx);
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    save();
}

void Mod::set_path(const std::string& new_path) {
    std::lock_guard lock(m_mtx);
    this->path = new_path;
    save();
    stop();
    start();
}