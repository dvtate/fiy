//
// Created by tate on 12/3/25.
//

// TODO PoC PoC -- Piece of crap proof of concept
//      needs cleaning up

#pragma once

#include <cstring>
#include <string_view>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <boost/beast/core/detail/base64.hpp>

#include "../../../util/CGI.hpp"
#include "../../../modlib/fediymod.hpp"
#include "../../../util/Crypto.hpp"

extern fiy::HostInfo g_host_info;

// inline std::string find_git_http_backend() {
//     // Look check all possible paths
//     for (const std::string& p : {
//         "/usr/lib/git-core/git-http-backend"
//     })
//         if (std::filesystem::exists(p)) {
//             g_host_info.log(4, "Found CGI plugin: " + p);
//             return p;
//         }
//
//     // Not found
//     g_host_info.log(0, "Could not find git-http-backend, are you sure git is installed?");
//     throw std::runtime_error("Could not find git-http-backend, are you sure git is installed?");
// }

inline std::string get_repo_path(const std::string_view path) {
    std::string ret = g_host_info.data_dir;
    ret += "/repos";
    const auto i = path.find(".git");
    if (i == std::string::npos) {
        g_host_info.log(1, "invalid repo path? " + std::string(path));
        ret += path;
        return ret;
    }
    ret += path.substr(0, i + 4);
    return ret;
}

void get_uri_components(
    const std::string_view path,
    std::string& SERVER_PROTOCOL,
    std::string& SERVER_NAME,
    std::string& SERVER_PORT,
    std::string& PATH_INFO,
    std::string& QUERY_STRING
) {
    // Protocol
    bool https = g_host_info.base_uri[4] == 's';
    SERVER_PROTOCOL = https ? "https" : "http";

    // Domain + Port
    std::string hostname = g_host_info.domain;
    const auto port_sep = hostname.find(':');
    SERVER_NAME = hostname.substr(0, port_sep);
    if (port_sep != std::string::npos) {
        SERVER_PORT = hostname.substr(
            port_sep + 1
            // , hostname.find('/', port_sep) - port_sep // This shouldn't be needed
        );
    } else {
        SERVER_PORT = https ? "443" : "80";
    }

    // Query string
    const auto qs = path.find('?');
    if (qs != std::string_view::npos)
        QUERY_STRING = path.substr(qs + 1);

    // PATH_INFO
    std::string_view subpath = g_host_info.base_uri + 8;
    subpath.remove_prefix(subpath.find('/'));
    if (subpath.empty() || subpath == "/") {
        PATH_INFO = path.substr(0, qs);
    } else {
        PATH_INFO = subpath;
        PATH_INFO += path;
    }

    // Server stuff
    SERVER_NAME = g_host_info.domain;
}

/*
 Response header fields
      header-field    = CGI-field | other-field
      CGI-field       = Content-Type | Location | Status
      other-field     = protocol-field | extension-field
      protocol-field  = generic-field
      extension-field = generic-field
      generic-field   = field-name ":" [ field-value ] NL
      field-name      = token
      field-value     = *( field-content | LWSP )
      field-content   = *( token | separator | quoted-string )
*/

inline fiy::Response parse_cgi_output(const std::string& s) {
    fiy::Response ret{200, nullptr, 0 };

    // Parse headers
    size_t start = 0;
    size_t end;
    do {
        end = s.find("\r\n", start);
        auto line = s.substr(start, end - start);
        if (line.empty()) {
            start = end + 2;
            break;
        }
        const auto i = line.find(':');
        if (i == std::string::npos) {
            std::cerr <<"WTF? invalid header? --" <<line << std::endl;
        }

        // Do something with the header
        auto header = line.substr(0, i);
        std::ranges::transform(header, header.begin(),
            [](const auto c){ return std::tolower(c); });
        if (header == "Status") {
            // This isn't a valid http header so we gotta handle it differently
            // Status         = "Status:" status-code SP reason-phrase NL
            ret.status = atoi(line.substr(i + 1, line.find(' ')).c_str()); // TODO use strtol + string_view
            if (ret.status == 0)
                ret.status = 500;
            std::cout <<" Status: " << ret.status << std::endl;
        } else if (header == "Content-Length") {
            // Extra Validation later
            ret.body_len = atoi(line.substr(i + 1 ).c_str());  // TODO use strtol + string_view
            ret.add_header(line);
            std::cout <<" Content-length" << ret.body_len << std::endl;
        } else {
            ret.add_header(line);
            std::cout <<"Header: " << line << std::endl;
        }
        start = end + 2;
    } while (end != std::string::npos && start < s.size() && s[start] != '\n');

    // Extract body
    if (end != std::string::npos) {
        // Body length
        ret.body = s.c_str() + start; // extra \r\n
        size_t body_len = s.size() - start - 1;
        if (ret.body_len != 0 && body_len != ret.body_len) {
            g_host_info.log(2, "Incorrect Content-Length header");
        }
        ret.body_len = body_len;
    }

    return ret;
}

/**
 * Wrapper around the git-http-backend CGI plugin included with git
 */
void git_repo_cgi(const fiy::Request& req, fiy_callback_t cb) {
    std::string_view path{req.path};

    CGI cgi{{"git", "http-backend" }};

    cgi.REQUEST_METHOD = req.method_str();
    cgi.CONTENT_TYPE = req.find_header("content-type");
    if (req.body != nullptr) {
        cgi.CONTENT_LENGTH = std::to_string(req.body_len);
        cgi.body = std::string(req.body, req.body_len);
    }
    cgi.CONTENT_LENGTH = req.find_header("content-length");
    cgi.set_env("GIT_HTTP_EXPORT_ALL", "1"); // not accepting raw paths so no worries
    cgi.set_env("GIT_HTTP_MAX_REQUEST_BUFFER", "1000M");

    get_uri_components(path,
        cgi.SERVER_PROTOCOL,
        cgi.SERVER_NAME,
        cgi.SERVER_PORT,
        cgi.PATH_INFO,
        cgi.QUERY_STRING);

    cgi.PATH_TRANSLATED = g_host_info.data_dir;
    cgi.PATH_TRANSLATED += "/repos";
    cgi.PATH_TRANSLATED += path.substr(0, path.find('?'));

    cgi.REMOTE_ADDR = req.domain ? req.domain : g_host_info.domain;
    cgi.REMOTE_HOST = "NULL";
    cgi.REMOTE_USER = req.user_str();

    // Add other http headers altho probably not needed
    for (const auto& [h , v] : req.headers_map()) {
        std::string ev = "HTTP_";
        ev += h;
        for (size_t i = 4; i < ev.size(); i++)
            if (ev[i] == '-')
                ev[i] = '_';
            else
                ev[i] = std::toupper(ev[i]);
        cgi.set_env(ev, std::string(v));
    }

    auto r = cgi.run();
    if (r.status != 0 || r.stdout.empty()) {
        req.respond(cb, 500, "Git CGI failed");
        std::cerr <<"\nstatus: " << r.status << std::endl;
        std::cerr <<"\nstdout: " << r.stdout << std::endl;
        std::cerr <<"err!\n";
        return;
    }

    std::cout << "--------------------------------------------\n";
    std::cout << "\nCGI Result: " <<r.stdout << std::endl;
    std::cout << "--------------------------------------------\n";
    req.respond(cb, parse_cgi_output(r.stdout));
}

void git_repo_auth(const fiy::Request& req, fiy_callback_t cb) {
    auto repo = get_repo_path(req.path);
    if (req.user && req.domain == nullptr) {
        // git http password auth handled by protocol server
    }

}