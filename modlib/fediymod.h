//
// Created by tate on 7/5/24.
//

#ifndef FEDIY_FEDIYMOD_H
#define FEDIY_FEDIYMOD_H

#ifndef __cplusplus
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#else
#include <cstdint>
#include <string>
#endif

#ifndef FEDIY_PROTOCOL_SERVER_SOURCE

/// String versions of http verb
static inline const char* fiy_http_verb_string(uint8_t verb) {
    // from boost::beast::http::verb
    static const char* verb_strings[] = {
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
    return verb_strings[(verb >= 34) ? 0 : verb];
}
#endif

/**
 * IPC request from a user
 */
struct fiy_request_t {
    /**
     * Relevant fediy instance
     */
    const char* domain;     // null = local user

    /**
     * User that made the request
     */
    const char* user;       // null = unauthenticated

    /**
     * Request path
     *
     * ie - "/post/create"
     */
    const char* path;

    /**
     * Selected headers in the user's request
     */
    const char* headers;
    /*
     * Headers we should probably include:
     * - Content-Type ?
     * - Content-Encoding ?
     * - Accept ?
     * - Accept-Encoding
     * - Cookie ?
     */

    /**
     * Request body
     */
    const char* body;       // null = get request

    /**
     * How big is the body
     * @note can't rely on null-terminators bc body could be binary format
     */
    size_t body_len;

    /**
     * Enum value corresponding to the HTTP verb sent
     */
    uint8_t method;   // http method (from boost::beast::http::verb)

};

struct fiy_response_t {
    /**
     * HTTP Status code
     */
    int status;

    /**
     * HTTP body
     */
    const char* body;

    /**
     * How many characters are in the body
     */
    size_t body_len;

    /**
     * HTTP Headers to set
     */
    const char* headers;
};

typedef void (* fiy_callback_t)(const struct fiy_request_t* request, const struct fiy_response_t*);

/// This is used to provide callbacks to the host
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

/// Some info from the host that may be relevant to the module
struct fiy_host_info_t {
    /**
     * Fediy instance domain
     */
    const char* domain;

    /**
     * unique app id for this app
     */
    const char* app_id;

    /**
     * Base uri for this app
     * - format: <protocol>://<host>/<appid>
     * - ie - https://bodge.dev/git
     */
    const char* base_uri;

    /**
     * Where this app's files go
     */
    const char* data_dir;

    /**
     * Write a debug message
     * @param level
     * @param message
     */
    void (*log)(int level, const char* message);

    /**
     * Send a request to another app
     * @param app_id app to send the request to
     * @param request request to send to the other app
     *      method   - http method
     *      path     - uri path
     *      domain   - remote server to send request to or nullptr if local inter-app request
     *      user     - local user or nullptr if unauthenticated
     * @param context this pointer is passed back to the callback at the end
     * @param callback called with response
     * @notes
     * - local apps can send requests to each other without restrictions
     * - an app on server a can only send requests to apps on server b on behalf of users residing on server a
     *    - this prevents false impersonation
     */
    void (*request)(
//    const struct fiy_host_info_t* host,
            const char* app_id,
            const struct fiy_request_t* request,
            void* context,
            void (*callback)(const struct fiy_response_t*, void*)
    );

    /**
     * Authenticate an instance-local user
     * @param username user's username
     * @param password user's password
     * @return true if valid credentials
     */
    bool (*local_login)(const char* username, const char* password);

    /**
     * Is the given user an admin on this fediy instance?
     * @param local_user_name
     * @return true if the user is an admin
     */
    bool (*is_admin)(const char* local_user_name);
};

typedef struct fiy_mod_info_t* (*fiy_mod_start_function_t)(const struct fiy_host_info_t*);



#endif //FEDIY_FEDIYMOD_H
