//
// Created by tate on 7/5/24.
//

#include <iostream>

#include "fediymod.hpp"

//////////////////////////
// Exports
////////////////////////

static const fiy_host_info_t* g_host_info;

static void handle_request(fiy_request_t* _request, fiy_callback_t callback) {
    auto req = (fiy::Request*) _request;

    std::string body = "<ul>";
    body += "<b>Method: </b>";
    body += req->method_str();
    body += "\n<br/>\n";
    body += "<b>Path: </b>";
    body += req->path;
    body += "\n<br/>\n";
    body += "<b>App: </b>";
    body += g_host_info->app_id;
    body += " @ ";
    body += g_host_info->domain;
    body += "\n<br/>\n";
    body += "<b>User: </b>";
    body += req->user_str("not logged in");
    body += "\n<br/>\n";
    body += "<b>Body: </b>";
    body += req->body;
    body += "</ul>\n<hr/>";

    printf("=====\nlib_cpp.so:\n%s : %s\nUser: %s\nDomain: %s\nBody: %s\nHeaders: %s\n=====\n",
           req->method_str(), req->path, req->user, req->domain,
           req->body, req->headers);

    req->respond(callback, 200, body, "Content-Type: text/html");
}

//
//// if domain is null, then user is local
//void (* username_change_handler)(const char* domain, const char* old_username, const char* new_username);
//
///// peer domain changed
//void (* peer_domain_change_handler)(const char* old_domain, const char* new_domain);
//


extern "C" fiy_mod_info_t* start(const fiy_host_info_t* host_info) {
    g_host_info = host_info;
    static fiy_mod_info_t mod_info = {
        .on_request=handle_request,
        .on_peer_domain_changed=nullptr,
        .on_username_changed=nullptr,
    };
    return &mod_info;
}
