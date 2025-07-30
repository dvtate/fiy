#include <string.h>
#include <stdlib.h>

#include "fediymod.h"

//////////////////////////
// Exports
////////////////////////

static void handle_request(struct fiy_request_t* request, fiy_callback_t callback) {
    // Allocate a body string to send to the user
    size_t body_len = 50
        + (request->user != NULL ? strlen(request->user) : 4)
        + (request->domain != NULL ? strlen(request->domain) : 4)
        + (10)
        + (request->path != NULL ? strlen(request->path) : 4);
    char* body = (char*) malloc(sizeof(char) * body_len);

    // Construct body string
    snprintf(
        body,
        body_len,
        "Hello, @%s@%s! <br/>Action: %s %s",
        request->user == NULL ? "null" : request->user,
        request->domain == NULL ? "null" : request->domain,
        fiy_http_verb_string(request->method),
        request->path == NULL ? "null" : request->path
    );

    // Pass response to callback
    struct fiy_response_t resp = {
        .status=200,
        .body=body,
        .body_len=strlen(body),
        .headers="Content-Type: text/html"
    };
    callback(request, &resp);

    // Cleanup
    free(body);
}

static void update_peer_domain(const char* old_domain, const char* new_domain) {
    printf("Peer moved from %s to %s\n", old_domain, new_domain);
}

static void update_username(const char* old_username, const char* new_username) {
    printf("User moved from %s to %s\n", old_username, new_username);
}

/**
 * Initialize the module so that it can begin accepting requests
 * @param domain
 * @param module_dir
 * @return
 */
struct fiy_mod_info_t* start(const struct fiy_host_info_t* host_info) {
    // Prepare and make sure everything is set up and installed correctly
    static struct fiy_mod_info_t mod_info = {
        .on_request=handle_request,
        .on_peer_domain_changed=update_peer_domain,
        .on_username_changed=update_username
    };
    return &mod_info;
}