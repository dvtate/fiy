//
// Created by tate on 2/20/26.
//

#include <fcntl.h>

#include <filesystem>
#include <fstream>
#include <optional>

#if defined __has_include
#  if __has_include (<linux/limits.h>)
#    include <linux/limits.h>
#  endif
#endif

#include <nlohmann/json.hpp>

#include "../../modlib/fiymod.hpp"
#include "../../util/mime.hpp"
#include "../../util/WebUtils.hpp"

namespace fs = std::filesystem;

/// Where to look for the files
static fs::path g_static_root;

/// Index files to use when directory requested
static std::vector<std::string> g_index_files{
    "index.html",
    "index.htm",
    "index.xhtml",
};

/// If a directory is chosen and it doesn't contain an index file,
/// should we list the contents of the directory?
static bool g_list_directory = true;

static fiy::Response file_response(const fs::path& path) {
    // Respond with the file
    // fiy::host().log_info("Requested file: " + path->string());
    fiy::Response res;
    const auto fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) [[unlikely]] {
        fiy::host().log_error("Could not open file " + path.string() + " : " + strerror(errno));
        res.status = 500;
        res.headers = "Content-Type: text/html";
        res.body = fiy::Body("<h1>Server Error</h1> <p>Could not open file!</p>");
        return res;
    }

    // posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    res.body = fiy::Body(fd, 0);
    res.status = 200;
    res.set_headers("Cache-Control: max-age=600\nContent-Type: "
        + std::string(get_ext_mime_type(path.extension().string().substr(1))));
    return res;

    // FIY Host closes the file when it's done. Don't close it here.
    // No need to close it here
    // if (close(fd) != 0)
    //     fiy::host().log_error("failed to close() file: " + f->string());
}


/**
 * Concatenate components into a single string, reserving space before appending them
 * @tparam Args char or StringViewLike
 * @param args parts to combine into single string
 * @return concatenated string
 */
template<typename... Args>
__attribute__((always_inline))
static constexpr inline std::string str_concat(const Args&... args) {
    constexpr auto to_sv = []<typename T>(const T& v) constexpr {
        if constexpr (std::is_same_v<T, char>)
            return std::string_view(&v, 1);
        else
            return std::string_view(v);
    };
    return [](const auto ...sv) constexpr {
        std::string ret;
        ret.reserve((sv.size() + ...));
        (ret.append(sv), ...);
        return ret;
    }(to_sv(args)...);
}

static std::string list_directory_body(const fs::path& path) {
    const std::string relative_path = fs::relative(path, g_static_root).string();

    // std::cout <<"relative( " <<g_static_root <<", " <<path <<") = " <<relative_path <<std::endl;;
    std::string body = str_concat(
        "<!DOCTYPE html><html><head><title>Index of ",
        relative_path,
        "</title><base href=\"",
        fiy::host().base_uri,
        '/',
        relative_path,
        "/\"></head><body><h1>Index of ",
        relative_path,
        "/</h1><hr><pre>"
    );

    for (const auto& entry : fs::directory_iterator(path)) {
        std::string p = entry.path().filename().string();
        if (entry.is_directory()) {
            body += str_concat(
                "<a href=\"./",
                WebUtils::uri_encode(p),
                "/\">",
                p,
                "/</a>\n"
            );
        } else {
            body += str_concat(
                "<a href=\"./",
                WebUtils::uri_encode(p),
                "\">",
                p,
                "</a>\n"
            );
        }
    }

    body += "</pre></body></html>";
    return body;
}

static void handle_request(fiy::Request& req, const fiy::fiy_callback_t cb) {
    try {
        static const fiy::Response response_404{
            404,
            "Content-Type: text/html",
            fiy::Body( "Error 404 - Not found" )
        };
        static const fiy::Response response_403{
            403,
            "Content-Type: text/html",
            fiy::Body( "Error 403 - Forbidden" )
        };

        // Strip query string and fragment
        const auto decoded_path = WebUtils::uri_decode(req.path, strlen(req.path));
#ifdef PATH_MAX
        if (decoded_path.size() >= PATH_MAX) {
            req.respond(cb, 400, "Content-Type: text/html",
                fiy::Body("Error 400: Path Length too long"));
            return;
        }
#endif
        std::string_view url_path = decoded_path;
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
            // fiy::host().log_debug("Directory escape attempted: " + std::string(normalized));
            req.respond(cb, response_403);
            return;
        }

        // File doesn't exist
        if (!fs::exists(normalized)) {
            // fiy::host().log_debug("File does not exist: " + std::string(normalized));
            req.respond(cb, response_404);
            return;
        }

        // If it's a directory, try index files
        if (fs::is_directory(normalized)) {
            for (const auto& index : g_index_files) {
                fs::path index_path = normalized / index;
                if (fs::exists(index_path) && fs::is_regular_file(index_path)) {
                    req.respond(cb, file_response(index_path));
                    return;
                }
            }

            if (g_list_directory) {
                const std::string body = list_directory_body(normalized);
                req.respond(cb, 200, "Content-Type: text/html", fiy::Body(body));
                return;
            }

            req.respond(cb, response_404);
            return;
        }

        // Must exist and be a regular file
        if (fs::is_regular_file(normalized)) {
            req.respond(cb, file_response(normalized));
            return;
        }

        // Maybe a pipe or fifo... let's just pretend it doesn't exist
        req.respond(cb, response_404);
        return;
    } catch (const fs::filesystem_error& e) {
        std::string body = str_concat("<h1>Server Error</h1>Filesystem error: ", e.what());
        req.respond(cb, 500, "Content-Type: text/html", fiy::Body(body));
        return;
    }
}

/// Export: Start module
FIY_EXPORT fiy::ModInfo* start(const fiy_host_info_t* host_info) {
    fiy::host() = *host_info;

    // Read config
    std::ifstream ifs{fiy::host().mod_config};
    auto config = nlohmann::json::parse(ifs);
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
    if (config.contains("list_directories")) {
        if (!config["list_directories"].is_boolean()) {
            fiy::host().log_error("module.json: list_directories should be boolean");
        } else {
            g_list_directory = config["list_directories"].get<bool>();
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