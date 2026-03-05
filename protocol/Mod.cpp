#include "Mod.hpp"


#include <dlfcn.h>
#include <fstream>
#include <filesystem>

#include <boost/unordered/unordered_flat_set.hpp>

#include "nlohmann/json.hpp"

#include "FIY.hpp"

[[nodiscard]] inline std::filesystem::path Mod::dir() const {
    return g_fiy->m_config.m_data_dir + "/mods/" + m_data_dir;
}

Mod::Mod(std::string data_dir) {
    m_data_dir = std::move(data_dir);
    m_user_data_dir = data_dir;
    load();
}

Mod::~Mod() {
    stop();
}

bool Mod::start() {
    std::lock_guard lock(m_mtx);

    // Note: this function can update our fields if not populated from module.json
    return m_running = m_ipc->start();
}

bool Mod::stop() {
    std::lock_guard lock(m_mtx);
    m_running = false;
    return m_ipc->stop();
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
    json["id"] = m_id;

    // This is the path that the users can use to access the mod (locally unique)
    json["path"] = m_path;

    // Mod version of the form X.y or just X
    json["version"] = m_version.str();

    // User-readable name for the mod
    json["name"] = m_name;

    // User-readable description of the mod and what it does
    json["description"] = m_description;

    // Mod icon
    json["icon"] = m_icon;

    // User can disable mods they don't want
    json["enabled"] = m_enabled;

    // How does this mod connect to the protocol server
    json["connector"] = m_ipc->type() == ModConnector::Type::NETWORK
                ? m_ipc->m_uri.starts_with("https")
                    ? "https"
                    : "http"
                : m_ipc->type() == ModConnector::Type::SHARED_LIBRARY
                    ? "shared_object"
                    : "socket";

    json["connector_uri"] = m_ipc->m_uri;
    return json.dump();
}

/**
 * @return Mod description for frontend
 */
std::string Mod::user_json() {
    nlohmann::json json = {
        { "id", m_id },
        { "path", m_path },
        { "version", m_version.str() },
        { "name", m_name },
        { "description", m_description },
        { "icon", m_icon },
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
 * @return
 */
Mod::AccessChecker Mod::parse_access_checker(const std::string& value) {
    // Global access
    if (value.empty() || value == "public")
        return [](const char*, const char*) {
            return true;
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
    if (m_id.empty())
        m_id = m_data_dir;
    if (m_path.empty())
        m_path = m_id;

    // Set error and log
    m_error.clear();

    // Load JSON from file
    auto conf = parse_file();

    // Override id
    if (conf.contains("id")) {
        auto conf_id = conf.at("id");
        if (!conf_id.is_string()) {
            load_error("module.json: \"id\" should be a string");
        } else {
            auto new_id = conf_id.get<std::string>();
            if (m_id != new_id)
                LOG_ERR(m_id << "/module.json: \"id\" field (" <<new_id
                    <<") does not match directory name, the mod may not work");
            m_id = new_id;
        }
    }

    // Get routing path
    if (conf.contains("path")) {
        auto path = conf.at("path");
        if (!path.is_string()) {
            load_error("module.json: \"path\" should contain a uri component string");
        } else {
            // TODO validate uri component
            m_path = path.get<std::string>();
        }
    }

    // Get name
    if (!conf.contains("name")) {
        load_error("module.json: missing key: name");
    } else {
        auto name = conf.at("name");
        if (!name.is_string()) {
            load_error("module.json: \"name\" should be a string");
        } else {
            m_name = name.get<std::string>();
        }
    }

    // Get version
    if (conf.contains("version")) {
        auto version = conf.at("version");
        if (!version.is_string()) {
            load_error("module.json: \"version\" should be a string of format NN.nn");
        } else {
            m_version = Mod::Version(version.get<std::string>());
        }
    } else {
        // load_error("module.json: missing key: version");
    }

    // Get description
    if (!conf.contains("description")) {
        load_error("module.json: missing key: description");
    } else {
        auto description = conf.at("description");
        if (!description.is_string()) {
            load_error("module.json: \"description\" should be a string");
        } else {
            m_description = description.get<std::string>();
        }
    }

    // Set up IPC (order matters)
    std::string connector_uri;
    if (conf.contains("connector_uri")) {
        auto p = conf.at("connector_uri");
        if (!p.is_string()) {
            load_error("module.json: \"connector_uri\" should be a string");
        } else {
            connector_uri = p.get<std::string>();
        }
    }
    if (!conf.contains("connector")) {
        load_error("module.json: missing key: connector");
    } else {
        auto t = conf.at("connector");
        std::string ts;
        if (!t.is_string()) {
            load_error("module.json: \"connector\" should be either \"shared_object\", \"socket\" or \"tcp\"");
        } else {
            ts = t.get<std::string>();
        }

        if (ts == "http" || ts == "https") {
            if (connector_uri.empty()) {
                load_error("module.json: \"connector_uri\" must be defined when \"ipc\" is set to \"" + ts + "\"");
            } else {
                m_ipc = std::make_unique<ModNetConnector>(this, connector_uri, ts == "https");
            }
        } else if (ts == "shared_object") {
            if (connector_uri.empty())
                connector_uri = dir() / "module.so";
            m_ipc = std::make_unique<ModDLLConnector>(this, connector_uri);
        } else if (ts == "socket") {
            if (connector_uri.empty())
                connector_uri = dir() / "ipc.sock";
//            m_ipc = std::make_unique<ModSockIPC>(connector_uri);
        }
    }

    if (conf.contains("enabled")) {
        auto enabled = conf.at("enabled");
        if (enabled.is_boolean()) {
            m_enabled = enabled.get<bool>();
        } else if (enabled.is_number()) {
            m_enabled = enabled.get<double>() != 0;
        } else {
            load_error("module.json: \"enabled\" should be a boolean");
        }
    } else {
        m_enabled = false;
    }

    if (conf.contains("icon")) {
        auto icon = conf.at("icon");
        if (icon.is_string()) {
            auto icon_path = icon.get<std::string>();
            if (icon_path[0] == '/')
                m_icon = icon_path;
            else
                m_icon = dir() / icon_path;
        } else if (!icon.is_null()) {
            load_error("module.json: \"icon\" should be a string containing the icon file name");
        }
    }

    if (conf.contains("access")) {
        auto a = conf.at("access");
        if (a.is_string()) [[likely]] {
            m_can_access = parse_access_checker(a.get<std::string>());
        } else {
            load_error("module.json: \"access\" should be either \"public\", \"federated\", \"local\","
                " or a comma-separated list of local users allowed to use the mod");
        }
    } else {
        m_can_access = parse_access_checker("");
    }

    if (conf.contains("data_dir")) {
        auto d = conf.at("data_dir");
        if (d.is_string()) [[likely]] {
            m_user_data_dir = d.get<std::string>();
        } else {
            load_error("module.json: \"data_dir\" should be a string");
        }
    } else {
        m_user_data_dir = dir();
    }

    // Get install ts
    try {
        m_install_ts = std::filesystem::last_write_time(dir());
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
    LOG_ERR(m_id << ": " <<message);
}

nlohmann::json Mod::parse_file() {
    const std::filesystem::path mp = dir();
    if (!std::filesystem::exists(mp / "module.json")) {
        load_error("missing module.json");
        return{};
    }

    // Load config
    m_config = Pages::load_file_as_string(mp / "module.json");
    auto ret = nlohmann::json::parse( m_config.begin(), m_config.end() );
    if (!ret.is_object()) {
        load_error("module.json: should be an object");
        return{};
    }
    return ret;
}

void Mod::set_enabled(const bool enabled) {
    std::lock_guard lock(m_mtx);
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    save();
}

void Mod::set_path(const std::string& path) {
    std::lock_guard lock(m_mtx);
    m_path = path;
    save();
    stop();
    start();
}