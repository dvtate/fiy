//
// Created by tate on 7/5/24.
//

#ifndef FEDIY_FEDIYMOD_H
#define FEDIY_FEDIYMOD_H

#ifndef __cplusplus
#include <stdio.h>
#else
#include <string>
#endif

#ifdef __cplusplus
namespace fediy {
#endif

/// String versions of http verbs
// from boost::beast::http::verb
const char* fiy_http_verb_strings[] = {
    "<unknown>",
    "DELETE",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "CONNECT",
    "OPTIONS",
    "TRACE",
    "COPY",
    "LOCK",
    "MKCOL",
    "MOVE",
    "PROPFIND",
    "PROPPATCH",
    "SEARCH",
    "UNLOCK",
    "BIND",
    "REBIND",
    "UNBIND",
    "ACL",
    "REPORT",
    "MKACTIVITY",
    "CHECKOUT",
    "MERGE",
    "M-SEARCH",
    "NOTIFY",
    "SUBSCRIBE",
    "UNSUBSCRIBE",
    "PATCH",
    "PURGE",
    "MKCALENDAR",
    "LINK",
    "UNLINK"
};

/**
 * IPC request from a user
 */
struct fiy_request_t {
    const char* path;
    const char* domain;     // null = local user
    const char* user;       // null = unauthenticated
    const char* headers;    // 
    const char* body;       // null = get request
    unsigned char method;   // http method (from boost::beast::http::verb)
};

struct fiy_response_t {
#ifndef __cplusplus
    int status;                     // exit status
    const char* body;               // body
    const char* headers;            // updated headers or null
#else
    int status{200};                // exit status
    const char* body{nullptr};      // body
    const char* headers{nullptr};   // updated headers or null
#endif
};

typedef void (* fiy_callback_t)(const struct fiy_request_t* request, const struct fiy_response_t*);

/// This is used to provide callbacks to the host app
struct fiy_mod_info_t {
    const char* remote_app_id;

    /// Handle http requests to the module
    void (*on_request)(struct fiy_request_t* request, fiy_callback_t callback);

    /// Peer domain changed
    void (*on_peer_domain_changed)(const char* old_domain, const char* new_domain);

    /// Username changed
    /// @note usernames of form user@domain
    void (*on_username_changed)(const char* old_username, const char* new_username);
};

/**
 * Send a request to another app
 * @param app_id app to send the request to
 * @param request request to send to the other app
 *      method   - http method
 *      path     - uri path
 *      domain   - remote server to send request to or nullptr if local inter-app request
 *      user     - local user or nullptr if unauthenticated
 * @param callback
 * @notes
 * - local apps can send requests to each other without restrictions
 * - an app on server a can only send requests to apps on server b on behalf of users residing on server a
 *    - this prevents false impersonation
 */
//struct fiy_host_info_t;
typedef void (*fiy_send_request_t)(
//    const struct fiy_host_info_t* host,
    const char* app_id,
    const struct fiy_request_t* request,
    void (*callback)(const struct fiy_response_t*)
);

struct fiy_host_info_t {
    const char* domain;
    const char* base_uri; // <protocol>://<host>/<appid> ie - https://bodge.dev/git
    void (*log)(int level, const char* message);
    fiy_send_request_t request;
};

typedef struct fiy_mod_info_t* (*fiy_mod_start_function_t)(const struct fiy_host_info_t*);

#ifdef __cplusplus
    struct ModInfo : public fiy_mod_info_t {};
//    struct Request : public fiy_request_t {};
//    struct HostInfo : public fiy_host_info_t {};
//    struct Response : public fiy_response_t {
//        std::string body;
//        std::string headers;
//        int status;
//        explicit Response(std::string body, int status = 200):
//            body(std::move(body)),
//            status(status)
//        {}
//
//        ~Response() {}
//
//        operator struct fiy_response_t() {
//            return (struct fiy_response_t){
//                .status=status,
//                .body=body.c_str(),
//                .headers=headers.c_str()
//            };
//        }
//    };
#endif


#ifdef __cplusplus
} // namespace fediy
#endif


#endif //FEDIY_FEDIYMOD_H
