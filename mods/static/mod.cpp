//
// Created by tate on 2/20/26.
//

#include <fcntl.h>

#include <filesystem>
#include <optional>

#include <nlohmann/json.hpp>

#include "../../modlib/fiymod.hpp"
#include "../../util/mime.hpp"

/// Where to look for the files
std::filesystem::path g_static_root;

/// Index files to use when directory requested
std::vector<std::string> g_index_files{
    "index.html",
    "index.htm",
    "index.xhtml",
};

/**
 * Determine what file to serve based on user request
 * @param url_path request path
 * @return if url_path is valid, returns resolved file path
 */
std::optional<std::filesystem::path> resolve_static_file(std::string_view url_path) {
    // Strip query string and fragment
    namespace fs = std::filesystem;
    auto pos = url_path.find_first_of("?#");
    if (pos != std::string::npos)
        url_path = url_path.substr(0, pos);

    // Remove leading '/'
    pos = url_path.find_first_not_of('/');
    if (pos != std::string::npos)
        url_path = url_path.substr(pos);
    else
        url_path = "";

    // Weakly canonical resolves ".." even if file doesn't exist
    fs::path normalized = fs::weakly_canonical(g_static_root / url_path);

    // Ensure path is still inside base directory
    const auto mismatch = std::mismatch(
        g_static_root.begin(), g_static_root.end(),
        normalized.begin(), normalized.end()
    ).first;
    if (mismatch != g_static_root.end()) {
        // fiy::host().log_debug("Directory traversal attempted: " + std::string(normalized));
        return std::nullopt; // attempted directory traversal
    }

    // File doesn't exist
    if (!fs::exists(normalized)) {
        // fiy::host().log_debug("File does not exist: " + std::string(normalized));
        return std::nullopt;
    }

    // If it's a directory, try index files
    if (fs::exists(normalized) && fs::is_directory(normalized)) {
        for (const auto& index : g_index_files) {
            fs::path index_path = normalized / index;
            if (fs::exists(index_path) && fs::is_regular_file(index_path))
                return index_path;
        }
        return std::nullopt;
    }

    // Must exist and be a regular file
    if (fs::exists(normalized) && fs::is_regular_file(normalized))
        return normalized;

    return std::nullopt;
}

void handle_request(fiy::Request& req, const fiy::fiy_callback_t cb) {
    const auto f = resolve_static_file(req.path);
    if (!f) {
        // 404
        req.respond(cb, 404,
            "Content-Type: text/html",
            "404 - Not found");
        return;
    }

    // Respond with the file
    // fiy::host().log_info("Requested file: " + f->string());
    fiy::Response res;
    const auto fd = open(f->c_str(), O_RDONLY);
    if (fd == -1) [[unlikely]] {
        fiy::host().log_error("Could not open file " + f->string() + " : " + strerror(errno));
        req.respond(cb, 500, "Content-Type: text/html",
            fiy::Body("<h1>Server Error</h1> <p>Could not open file!</p>"));
        return;
    }
    // posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    res.body = fiy::Body(fd, 0);
    res.status = 200;
    res.set_headers("Cache-Control: max-age=600\nContent-Type: "
        + std::string(get_ext_mime_type(f->extension().string().substr(1))));
    req.respond(cb, res);
    if (close(fd) != 0)
        fiy::host().log_error("failed to close() file: " + f->string());
}

/// Export: Start module
FIY_EXPORT fiy::ModInfo* start(const fiy_host_info_t* host_info) {
    // Read config
    fiy::host() = *host_info;
    auto config = nlohmann::json::parse(fiy::host().mod_config);
    if (!config.contains("mod_settings") || !config["mod_settings"].is_object()) {
        fiy::host().log_fatal("module.json: expected a mod_settings field containing an object with target path");
        return nullptr;
    }
    config = config["mod_settings"];
    if (!config.contains("root") || !config["root"].is_string()) {
        fiy::host().log_fatal("module.json: expected a mod_settings 'root' field set to string path to static root");
        return nullptr;
    }
    g_static_root = config["root"].get<std::string>();
    if (g_static_root.is_relative())
        g_static_root = fiy::host().data_dir / g_static_root;

    try {
        g_static_root = std::filesystem::absolute(g_static_root);
        if (!std::filesystem::exists(g_static_root)) {
            fiy::host().log_fatal("module.json: target path does not exist: " + std::string(g_static_root));
            return nullptr;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        fiy::host().log_fatal("module.json: Invalid target "
            + std::string(g_static_root) + ": " + e.what());
        return nullptr;
    }
    if (config.contains("index")) {
        if (!config["index"].is_array()) {
            fiy::host().log_error("module.json: mod_settings 'index' field should be an array of index filename strings");
        } else {
            try {
                g_index_files = config["index"].get<std::vector<std::string>>();
            } catch (...) {
                fiy::host().log_error("module.json: Invalid mod_settings.index field");
            }
        }
    }

    // Exchange info with host
    static fiy::ModInfo mod_info = {
        .on_request = [](fiy_request_t* r, const fiy::fiy_callback_t cb) {
            handle_request(static_cast<fiy::Request&>(*r), cb);
        },
        .delete_user = nullptr
    };
    return &mod_info;
}