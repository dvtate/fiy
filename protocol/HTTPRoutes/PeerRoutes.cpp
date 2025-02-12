//
// Created by tate on 6/25/24.
//

#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#include "drogon/HttpClient.h"

#include "App.hpp"

#include "PeerRoutes.hpp"

void PeerRoutes::pubkey(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    static const auto ret = drogon::HttpResponse::newFileResponse(
        g_app->m_config.m_data_dir + "/auth/pubkey"
    );
    callback(ret);
}
//
//std::string read_pubkey() {
//
//}
//
//std::string decrypt_message(std::string& msg) {
//
//}
//
//// same content both ways
//struct HandshakeMsg {
//    bool sig_valid;
//    std::string domain;
//    std::string token;
//    std::string symkey;
//
//    std::string to_string() {
//        return this->domain + "\n" + this->token + "\n" + this->symkey;
//    }
//    static HandshakeMsg from_string(std::string&& str) {
//        HandshakeMsg ret;
//
//    }
//};
//
//
//
//HandshakeMsg parse_handshake(std::string body) {
//    body = drogon::utils::base64Decode(body);
//
//}
//std::string write_handshake(HandshakeMsg msg);

std::vector<std::string> split_string(const std::string_view& str,
                                      const std::string& delimiter)
{
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos)
    {
        strings.emplace_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.size();
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.emplace_back(str.substr(prev));

    return strings;
}

void PeerRoutes::handshake(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    // TODO this algorithm is insecure and just used for testing
    auto parts = split_string(req->body(), "\n");
    auto domain = parts[0];
    auto token = parts[1];

    std::cout <<"New peer: " <<domain <<" : " <<token <<std::endl;

    // Add peer with unique auth token
    std::shared_ptr<Peer> p;
    do {
        p = std::make_shared<Peer>(domain, PeerAuth{"symkey", token});
    } while (!g_app->m_peers.add_peer(domain, p));

    // Respond with token
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setBody(p->m_auth.m_bearer_token_we_accept);
    callback(resp);

    // Actual algorithm to use:
    // 1. Use private key associated with our pubkey endpoint to decrypt request body
    // 2. Store provided peer data: host, symkey, tokens, etc. into Peers cache
    // 3. Send token + symkey, encrypted with the other peer's pubkey

    // Decrypt body
//    auto encryptedBody = drogon::utils::base64Decode(req->body()); // may contain \0

    // Get relevant public key
//    auto client = drogon::HttpClient::newHttpClient(remote_peer);
//    client.request();

}