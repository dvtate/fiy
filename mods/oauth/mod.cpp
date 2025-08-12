#include <deque>
#include <string>


#include "../../modlib/fediymod.hpp"



const fiy_host_info_t* g_host_info;


void handle_request(struct fiy_request_t* request, fiy_callback_t cb) {
    auto& req = *(fiy::Request*) request;

    switch ((fiy::Request::Method)req.method) {
        case fiy::Request::Method::GET:
            
            break;

        case fiy::Request::Method::POST: {
            std::string username, password;
            // 
            bool ok = g_host_info->local_login(username.c_str(), password.c_str());
            break;
        };
        default:
            // 404
            break;
    }
}


extern "C" fiy_mod_info_t* start(const fiy_host_info_t* host_info) {
    static fiy_mod_info_t mod_info = {
        .on_request=handle_request,
        .on_peer_domain_changed=nullptr,
        .on_username_changed=nullptr,
    };
    g_host_info = host_info;
    return &mod_info;
}
