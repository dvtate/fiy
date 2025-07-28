
#include "App.hpp"

#include "Server/util.hpp"

#include "Peers.hpp"


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
    DEBUG_LOG("Sending message to new peer...");
    // Generate unique auth token
    auto tok = PeerAuth::get_token_string();
    m_mtx.read_lock();
    while (m_peers_in.contains(tok))
        tok = PeerAuth::get_token_string();
    m_mtx.read_to_write();
    m_peers_in[tok] = nullptr;
    m_mtx.write_unlock();

    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::post);
    req.target("/peer/handshake");
    // TODO this is just a placeholder for now, actual algorithm in Server/Router.cpp
    req.body() = g_app->m_config.m_hostname + std::string("\n") + tok;
    req.keep_alive(false);
    req.prepare_payload();

    auto callback =
        [this, cb2 = std::move(cb), domain, token = std::move(tok)]
        (boost::beast::http::response<boost::beast::http::string_body> res)
    {
        // Make Peer
        auto p = std::make_shared<Peer>(
            domain,
            PeerAuth("symkey", std::string(res.body()), std::move(token))
        );

        // Add peer
        m_mtx.write_lock();
        m_peers_in[p->m_auth.m_bearer_token_we_accept] = p;
        auto ret = m_peers_out.emplace(domain, p);
        if (!ret.second) {
            DEBUG_LOG("Duplicate peer?? - " <<domain);
            m_peers_in.erase(p->m_auth.m_bearer_token_we_accept);
            m_mtx.write_unlock();
            return;
        }
        m_mtx.write_unlock();

        // Success
        DEBUG_LOG("New peer: " <<domain);
        cb2(p);
    };

    bool use_https = domain.find(':') == std::string::npos;
    if (use_https) {
        g_app->m_https.request(domain, req, callback);
    } else {
        g_app->m_http.request(domain, req, callback);
    }
}



struct ScopedRequest {
    std::string m_domain;
    std::string m_user;
    std::string m_path;
    std::string m_headers;
    std::string m_body;
    uint8_t m_method;

    explicit ScopedRequest(const fiy_request_t* req) {
        if (req->domain != nullptr)
            m_domain = req->domain;
        if (req->user != nullptr)
            m_user = req->user;
        if (req->path != nullptr)
            m_path = req->path;
        if (req->headers != nullptr)
            m_headers = req->headers;
        if (req->body != nullptr)
            m_body = req->body;
        m_method = req->method;
    }

    fiy_request_t temp_copy() const {
        return fiy_request_t{
                .domain=m_domain.empty() ? nullptr : m_domain.c_str() ,
                .user=m_user.empty() ? nullptr : m_user.c_str(),
                .path=m_path.empty() ? nullptr : m_path.c_str(),
                .headers=m_headers.empty() ? nullptr : m_headers.c_str(),
                .body=m_body.empty() ? nullptr : m_body.c_str(),
                .method=m_method,
        };
    }
};


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
            [appid, user, request = ScopedRequest(req), callback, context]
            (const std::shared_ptr<Peer>& p) {
                if (p != nullptr) {
                    DEBUG_LOG("sending request to peer " <<p->m_domain );
                    auto r = request.temp_copy(); // copied earlier to prevent invalidation
                    request_peer(p, appid, user, &r, context, callback);
                } else {
                    DEBUG_LOG("couldn't link with peer");
                    if (callback != nullptr)
                        callback(nullptr, context);
                }
            }
        );
        return;
    } else {
        request_peer(p, appid, user, req, context, callback);
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
    boost::beast::http::request<boost::beast::http::string_body> request;
    request.method((boost::beast::http::verb)req->method);
    request.target("/mods/" + appid);
    request.body() = req->body;
    request.set("Fiy-Peer", peer->m_auth.m_bearer_token_we_send);
    request.set("Fiy-User", user);
    request.set("Fiy-Path", req->path);
    response_set_headers(request, req->headers);
    request.prepare_payload();

    std::cout <<request <<std::endl;

    bool use_https = std::string_view(peer->m_domain).find(':') == std::string_view::npos;
    auto cb = [context, callback] (boost::beast::http::response<boost::beast::http::string_body> res) {
        if (callback == nullptr)
            return;
        auto headers_str = get_headers_string(res);
        const fiy_response_t response{
            .status = (int) res.result(),
            .body = res.body().c_str(),
            .headers = headers_str.c_str(),
        };
        callback(&response, context);
    };
    if (use_https) {
        g_app->m_https.request(peer->m_domain, std::move(request), std::move(cb));
    } else {
        g_app->m_http.request(peer->m_domain, std::move(request), std::move(cb));
    }

}