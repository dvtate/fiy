//
// Created by tate on 6/25/24.
//

#pragma once

#include "drogon/HttpController.h"

/*

# Peer Authentication:
## Process
- get peer's public key from endpoint
	- /auth/pub -> peer.key
- encrypted with peer.key, we send our public key, domain, and token to peer ; request signed with our pubkey
	- /auth/connect -> auth token, symmetric key
	- peer checks that provided public key corresponds with our /auth/pub key
	- peer sends back encrypted token along with symmetric key
-

- peer gets public certificate from server
- peer sends request to get signed token from peer
	- peer can reject this request
*/

class PeerRoutes : public drogon::HttpController<PeerRoutes, false> {
public:
    PeerRoutes() = default;

    // Send our public key
    static void pubkey(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    // Establish peer relationship with peer that made request
    static void handshake(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

//    METHOD_LIST_BEGIN
//        ADD_METHOD_TO(PeerRoutes::pubkey, "/peer/key", drogon::HttpMethod::Get, "ModSdCheckMiddleware");
//        ADD_METHOD_TO(PeerRoutes::handshake, "/peer/handshake", drogon::HttpMethod::Post, "ModSdCheckMiddleware");
//    METHOD_LIST_END
};
