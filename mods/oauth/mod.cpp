#include <deque>
#include <string>

#include "../../modlib/fediymod.hpp"

fiy::HostInfo g_host_info;

void handle_request(struct fiy_request_t* request, fiy::Callback cb) {
    auto& req = *(fiy::Request*) request;

    switch ((fiy::Request::Method)req.method) {
        case fiy::Request::Method::GET:
            
            break;

        case fiy::Request::Method::POST: {
            std::string username, password;
            // 
            bool ok = g_host_info.local_login(username.c_str(), password.c_str());
            break;
        };
        default:
            // 404
            break;
    }
}

extern "C" fiy::ModInfo* start(const fiy_host_info_t* host_info) {
    static fiy::ModInfo mod_info = {
        .on_request = handle_request,
        .delete_user = nullptr,
        .id = "fiy.oauth",
        .version = "0.0"
    };
    g_host_info = *host_info;
    return &mod_info;
}
