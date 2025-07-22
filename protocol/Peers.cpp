#include "drogon/HttpClient.h"
#include "Peers.hpp"


#include "App.hpp"

Peers::Peers() {
    m_cron = std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(
                    std::chrono::seconds(60 * 30));
            this->prune();
        }
    });
}

/**
 * @param domain
 * @param p
 * @return true when peer was added
 */
bool Peers::add_peer(const std::string& domain, const std::shared_ptr<Peer>& p) {
    RWMutex::LockForWrite lock{m_mtx};
    auto ret = m_peers_out.emplace(domain, p);
    if (!ret.second)
        return false;
    ret = m_peers_in.emplace(p->m_auth.m_bearer_token_we_accept, p);
    if (!ret.second) {
        m_peers_out.erase(domain);
        return false;
    }
    DEBUG_LOG("added peer: " << domain);
    return true;
}

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
    std::erase_if(m_peers_in, [this, now](const auto& item) {
        const auto& [domain, peer] = item;
        if (peer->m_auth.is_expired(now)) {
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
    const fiy_request_t* req,
    void* context,
    void (*callback)(const fiy_response_t*, void*)
) {
    if (domain == g_app->m_config.m_hostname) {
        std::cout <<"request_peer(localhost)\n";
        g_app->m_mods.get_mod(appid)->m_ipc->handle_request(req, context, callback);
    }

    auto p = get_peer_for_domain(domain);
    if (p == nullptr) {
        std::cout <<"peer not in cache\n";
        new_peer(
            domain,
            [appid, user, req, cb = std::move(callback), context]
            (const std::shared_ptr<Peer>& p) mutable
            {
                if (p != nullptr) {
                    DEBUG_LOG("sending request to peer " <<p->m_domain );
                    request_peer(p, appid, user, req, context, cb);
                } else {
                    DEBUG_LOG("couldn't link with peer");
                    cb(nullptr, context);
                }
            }
        );
        return;
    } else {
        request_peer(p, appid, user, req, context, callback);
    }
}


drogon::HttpMethod http_verb_boost_to_drogon(boost::beast::http::verb m) {
    using namespace boost::beast::http;
    switch (m) {
    case verb::get:     return drogon::HttpMethod::Get;
    case verb::post:    return drogon::HttpMethod::Post;
    case verb::head:    return drogon::HttpMethod::Head;
    case verb::put:     return drogon::HttpMethod::Put;
    case verb::delete_: return drogon::HttpMethod::Delete;
    case verb::options: return drogon::HttpMethod::Options;
    case verb::patch:   return drogon::HttpMethod::Patch;
    case verb::copy:    return drogon::HttpMethod::Copy;
    default:            return drogon::HttpMethod::Invalid;
    }
}

void Peers::request_peer(
    const std::shared_ptr<Peer>& peer,
    const std::string& appid,
    const std::string& user,
    const fiy_request_t* req,
    void* context,
    void (*callback)(const fiy_response_t*, void*)
) {
    auto client = drogon::HttpClient::newHttpClient(std::string("https://") + peer->m_domain);
    auto req2 = drogon::HttpRequest::newHttpRequest();
    req2->addHeader("Fiy-Peer", peer->m_auth.m_bearer_token_we_send);
    req2->addHeader("Fiy-User", user);
    req2->addHeader("Fiy-Path", req->path);
    req2->setPath("/mods/" + appid);
    req2->setMethod(http_verb_boost_to_drogon((boost::beast::http::verb)req->method));
    client->sendRequest(
        req2,
        [cb = std::move(callback), context]
        (
            drogon::ReqResult status,
            const drogon::HttpResponsePtr& resp
        ){
            if (status != drogon::ReqResult::Ok) {
                DEBUG_LOG("remote request failed: " << to_string(status));
                if (resp == nullptr) {
                    DEBUG_LOG("response is null!");
                    cb(nullptr, context);
                    return;
                }
            }
            fiy_response_t r {
                .status = resp->getStatusCode(),
                .body = resp->body().data()
            };
            cb(&r, context);
        }
    );
}