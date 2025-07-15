//
// Created by tate on 7/5/24.
//

#include "fediymodpp.hpp"

//////////////////////////
// Exports
////////////////////////

static void handle_request(fiy_request_t* _request, fiy_callback_t callback) {
    auto req = (fiy::Request*) _request;

    std::string body = "Hello, @";
    body += req->user_str("anon");
    body += "! <br/>Path: ";
    body += req->path;
    body += " ";
    if (req->path != nullptr)
        body += req->path;

    printf("=====\nlib_cpp.so:\n%s : %s\nUser: %s\nDomain: %s\nBody: %s\nHeaders: %s\n=====\n",
           req->method_str(), req->path, req->user, req->domain,
           req->body, req->headers);

    req->respond(callback, fiy::Response(200, body.c_str()));
}

//
//// if domain is null, then user is local
//void (* username_change_handler)(const char* domain, const char* old_username, const char* new_username);
//
///// peer domain changed
//void (* peer_domain_change_handler)(const char* old_domain, const char* new_domain);
//


extern "C" fiy_mod_info_t* start(const fiy_host_info_t* host_info) {
    static fiy_mod_info_t mod_info = {
        .on_request=handle_request,
        .on_peer_domain_changed=nullptr,
        .on_username_changed=nullptr,
    };
    return &mod_info;
}
