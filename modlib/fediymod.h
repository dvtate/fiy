//
// Created by tate on 7/5/24.
//

#ifndef FEDIY_FEDIYMOD_H
#define FEDIY_FEDIYMOD_H

#ifndef __cplusplus
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#else
#include <cstdint>
#include <string>
#include <ctime>
#endif

/// String versions of http verb
extern const char* fiy_http_verb_strings[];

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
     * Enum value corresponding to the HTTP verb sent
     */
    uint8_t method;   // http method (from boost::beast::http::verb)

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
     * How big is the body
     * @note can't rely on null-terminators bc body could be binary format
     */
    size_t body_len;

    /**
     * Request body
     */
    const char* body;       // null = get request
};

struct fiy_buffer_t {
    char* data;
    size_t size;
};

struct fiy_buffer_generator_t {
    // Have we reached the end of the buffer
    bool (*is_done)(struct fiy_buffer_generator_t*);

    // Get null-terminated array of buffers
    struct fiy_buffer_t* (*prepare)(struct fiy_buffer_generator_t*);

    // Reader is ready for another call to prepare
    void (*consume)(struct fiy_buffer_generator_t*);
};

#define FIY_BODY_TYPE_TEXT 0
#define FIY_BODY_TYPE_FILE 1
#define FIY_BODY_TYPE_BUFF 2


struct fiy_response_t {
    /**
     * HTTP Status code
     */
    int status;

    /**
     * HTTP Headers to set
     */
    const char* headers;

    /**
     * What type of body do we have?
     * 0 - text body
     * 1 - file
     * 2 - buffer generator
     */
    // uint8_t body_type : 2;

    /**
     * How many characters are in the body
     * -1 to skip Content-Length field
     */
    size_t body_len : 62;

    /**
     * HTTP body
     */
    const char* body;
    // union body {
    //     const char* body_text;
    //     int body_file;
    //     struct fiy_buffer_generator_t* generator;
    // };
};

typedef void (* fiy_callback_t)(const struct fiy_request_t* request, const struct fiy_response_t*);

/// This is used to provide callbacks to the host
struct fiy_mod_info_t {
    /// Handle http requests to the module
    void (*on_request)(struct fiy_request_t* request, fiy_callback_t callback);

    /// Called by host to delete all data associated with a deleted user
    /// @note username of form user@domain
    void (*delete_user)(const char* username);

    ////
    // These can be overriden by user's module.json
    ////

    /**
     * Globally unique mod id
     */
    const char* id
#ifdef __cplusplus
        {nullptr}
#endif
    ;

    /**
     * Version
     */
    const char* version
#ifdef __cplusplus
        {nullptr}
#endif
    ;
};

struct fiy_local_user_info_t {
    bool admin;
    int64_t join_ts;
    char name[200];   // max should be 128
    char locale[16]; // max should be 12
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
     *  - 0 : fatal
     *  - 1 : error
     *  - 2 : warning
     *  - 3 : info
     *  - 4 : debug
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
     * @return
     *      0 if valid credentials
     *      1 invalid credentials
     *      2 user locked
     *      -1 error
     */
    int (*local_login)(const char* username, const char* password);

    /**
     * Get information for a given local user
     * @param local_user_name username for relevant user
     * @param ret struct to put the results into
     * @return
     *  0 on success
     *  1 if the user does not exist
     *  -1 error
     */
    int (*user_info)(const char* local_user_name, struct fiy_local_user_info_t* ret);

    /**
     * Gets the current time according to the host
     */
    time_t (*now)();

    // TODO also send module.json contents
    const char* mod_config;
};

typedef struct fiy_mod_info_t* (*fiy_mod_start_function_t)(const struct fiy_host_info_t*);



#endif //FEDIY_FEDIYMOD_H
