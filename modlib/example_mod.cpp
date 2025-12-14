//
// Created by tate on 7/5/24.
//

#include <iostream>

#include "fediymod.hpp"

//////////////////////////
// Exports
////////////////////////

static fiy::HostInfo g_host_info;

static void handle_request(fiy::fiy_request_t* _request, fiy::fiy_callback_t callback) {
    const auto* req = (fiy::Request*) _request;

    std::string body_str = "<ul>";
    body_str += "<b>Method: </b>";
    body_str += req->method_str();
    body_str += "\n<br/>\n";
    body_str += "<b>Path: </b>";
    body_str += req->path;
    body_str += "\n<br/>\n";
    body_str += "<b>App: </b>";
    body_str += g_host_info.app_id;
    body_str += " @ ";
    body_str += g_host_info.domain;
    body_str += "\n<br/>\n";
    body_str += "<b>User: </b>";
    body_str += req->user_str("not logged in");
    body_str += "\n<br/>\n";
    body_str += "<b>Body: </b>";
    body_str += req->body;
    body_str += "</ul>\n<hr/>";

    printf("=====\nlib_cpp.so:\n%s : %s\nUser: %s\nDomain: %s\nBody: %s\nHeaders: %s\n=====\n",
           req->method_str(), req->path, req->user, req->domain,
           req->body, req->headers);

    req->respond(callback, 200,
        "Content-Type: text/html",
        fiy::Body(body_str));
}

extern "C" fiy::ModInfo* start(const fiy::fiy_host_info_t* host_info) {
    g_host_info = *host_info;
    static fiy::ModInfo mod_info = {
        .on_request=handle_request,
        .delete_user = [](const char* username) {
            std::cout <<"User deleted: " <<username <<std::endl;
        },
        .id = "example.cpp",
        .version = "0.0"
    };
    return &mod_info;
}

/// Export that gets called for the mod to do any cleanup before exit/dlclose
void stop() {
    g_host_info.log(fiy::HostInfo::Log::INFO, "Mod stopped");
}