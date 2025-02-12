#include "drogon/HttpClient.h"
#include "Peers.hpp"


#include "App.hpp"


/**
 * \returns null when not a peer invalid user token
 */
std::shared_ptr<Peer> Peers::get_peer_for_domain(const std::string& domain) {
    RWMutex::LockForRead lock{m_mtx};

    auto it = m_peers_out.find(domain);
    if (it != m_peers_out.end())
        return it->second;
    return nullptr;
}

std::shared_ptr<Peer> Peers::get_peer_from_token(const std::string& token) {
    RWMutex::LockForRead lock{m_mtx};

    auto it = m_peers_in.find(token);
    if (it != m_peers_in.end())
        return it->second;
    return nullptr;
}

void Peers::prune() {
    RWMutex::LockForWrite lock{m_mtx};
    const auto now = std::time(nullptr);
    std::erase_if(m_peers_in, [this](const auto& item) {
        const auto& [domain, peer] = item;
        if (peer->m_auth.is_expired()) {
            m_peers_out.erase(peer->m_domain);
            return true;
        }
        return false;
    });
}


void Peers::new_peer(const std::string& domain, std::function<void(const std::shared_ptr<Peer>&)> cb) {
    auto client = drogon::HttpClient::newHttpClient("http://" + domain);

    // Generate unique auth token
    auto tok = PeerAuth::get_token_string();
    m_mtx.read_lock();
    while (m_peers_in.contains(tok))
        tok = PeerAuth::get_token_string();
    m_mtx.read_unlock();

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setBody(g_app->m_config.m_hostname + std::string("\n") + tok);
    req->setPath("/peer/handshake");
    req->setMethod(drogon::HttpMethod::Post);
    client->sendRequest(
        req,
        [this, cb, domain, token = std::move(tok)]
        (
            drogon::ReqResult status,
            const drogon::HttpResponsePtr& resp
        ) {
            if (resp == nullptr) {
                std::cout <<"new peer status: " <<to_string(status) <<std::endl;
                DEBUG_LOG("failed to link with peer: " <<domain);
                cb(nullptr);
                return;
            }

            // TODO FIXME Race condition! another token could form between when we checked earlier and now
            auto p = std::make_shared<Peer>(domain, PeerAuth("symkey", std::string(resp->body()), token));
            this->add_peer(domain, p);
            cb(p);
        }
    );
}

void Peers::request_peer(
    const std::string& domain,
    const std::string& appid,
    const std::string& user,
    drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)> callback
) {
    if (domain == g_app->m_config.m_hostname) {
        std::cout <<"request_peer(localhost)\n";
        g_app->m_mods.get_mod_by_id(appid)->m_ipc->handle_request(
            req,
            ModuleRoutes::User {.domain=nullptr, .user=user},
            std::move(callback)
        );
    }

    auto p = get_peer_for_domain(domain);
    if (p == nullptr) {
        std::cout <<"peer not in cache\n";
        new_peer(
            domain,
            [appid, user, req, cb = std::move(callback)]
            (const std::shared_ptr<Peer>& p) mutable
            {
                if (p != nullptr) {
                    DEBUG_LOG("sending request to peer " <<p->m_domain );
                    request_peer(p, appid, user, req, cb);
                } else {
                    DEBUG_LOG("couldn't link with peer");
                    cb(nullptr);
                }
            }
        );
        return;
    } else {
        request_peer(p, appid, user, req, callback);
    }
}
void Peers::request_peer(
    const std::shared_ptr<Peer>& peer,
    const std::string& appid,
    const std::string& user,
    drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)> callback
) {
    auto client = drogon::HttpClient::newHttpClient(std::string("http://") + peer->m_domain);
    req->addHeader("Fediy-Peer", peer->m_auth.m_bearer_token_we_send);
    req->addHeader("Fediy-User", user);
    req->addHeader("Fediy-Path", req->getPath());
    req->setPath("/mods/" + appid + "/msg");
    client->sendRequest(
        req,
        [cb = std::move(callback)]
        (
            drogon::ReqResult status,
            const drogon::HttpResponsePtr& resp
        ){
            if (status != drogon::ReqResult::Ok) {
                DEBUG_LOG("remote request failed: " << to_string(status));
                if (resp == nullptr) {
                    DEBUG_LOG("response is null!");
                }
            }
            cb(resp);
        }
    );
}