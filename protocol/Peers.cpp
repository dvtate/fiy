#include "FIY.hpp"

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
 * Remove a peer from cache
 * @param domain peer domain to remove from cache
 * @return true if erased, false otherwise
 */
bool Peers::remove_peer(const std::string& domain) {
    RWMutex::LockForWrite lock{m_mtx};
    const auto it = m_peers_out.find(domain);
    if (it == m_peers_out.end() || it->second == nullptr)
        return false;
    m_peers_in.erase(it->second->m_auth.m_bearer_token_we_accept);
    m_peers_out.erase(it);
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

    if (const auto it = m_peers_in.find(token); it != m_peers_in.end())
        return it->second;
    return nullptr;
}

void Peers::prune() {
    RWMutex::LockForWrite lock{m_mtx};
    const auto now = g_fiy->now();
    std::erase_if(m_peers_in, [this, now](const auto& item) {
        const auto& [domain, peer] = item;
        if (peer == nullptr)
            return false; // would be unsafe to erase placeholders
        if (peer->m_auth.is_expired(now)) {
            m_peers_out.erase(peer->m_domain);
            return true;
        }
        return false;
    });
}

// TODO maybe prevent this from getting called multiple times for same peer
void Peers::new_peer(const std::string& domain, std::function<void(const std::shared_ptr<Peer>&)> cb) {
    DEBUG_LOG("Sending message to new peer " << domain);

    auto with_key_cb =
        [this, domain, cb = std::move(cb)]
        (boost::beast::http::response<boost::beast::http::string_body> res)
    {
        // Get key from response
        if (res.result() != boost::beast::http::status::ok || res.body().empty()) {
            cb(nullptr);
            return;
        }
        const auto& key = res.body();

        // Generate unique auth token
        auto bearer_token = PeerAuth::get_token_string();
        m_mtx.write_lock();
        while (m_peers_in.contains(bearer_token))
            bearer_token = PeerAuth::get_token_string();
        m_peers_in[bearer_token] = nullptr; // placeholder to prevent another peer from using this token
        m_mtx.write_unlock();

        std::string symkey_part1 = PeerAuth::get_token_string();

        std::string payload;
        payload += g_fiy->m_config.m_hostname;
        payload += '\n';
        payload += symkey_part1;
        payload += '\n';
        payload += bearer_token;
        auto signature = Crypto::SSL::sign(g_fiy->m_config.m_private_key, payload);
        payload += '\n' + signature;
        // DEBUG_LOG("Outgoing Handshake request:\n" << payload << "\n");

        // auto encrypted_payload = Crypto::SSL::encrypt(key, payload);
        // instead: symmetrically encrypt body payload, pk encrypt symkey in header

        // Make handshake request
        boost::beast::http::request<boost::beast::http::string_body> req;
        req.method(boost::beast::http::verb::post);
        req.target("/peer/handshake");
        // req.body() = encrypted_pyload;
        req.body() = payload;
        req.keep_alive(false);
        req.prepare_payload();

        auto handshake_cb = [
            this,
            key = std::move(key),
            cb2 = std::move(cb),
            domain,
            secret = std::move(symkey_part1),
            token = std::move(bearer_token)
        ] (
            boost::beast::http::response<boost::beast::http::string_body> res
        ) mutable {
            // Check status
            if (res.result_int() != 200) {
                LOG_ERR("Handshake failed: " << res.result_int() << " -- " << res.body());
                cb2(nullptr);
                return;
            }

            // Extract body components
            // auto body = Crypto::SSL::decrypt(g_fiy->m_config.m_private_key, res.body());
            const auto& body = res.body();
            // DEBUG_LOG("Handshake response:\n" << body << "\n");

            auto eol = body.find('\n');
            if (eol == std::string::npos) {
                LOG_ERR("Invalid handshake response: no bearer token");
                cb2(nullptr);
                return;
            }
            std::string bearer_token_we_use = body.substr(0, eol);
            auto start = eol + 1;
            eol = body.find('\n', start);
            if (eol == std::string::npos) {
                LOG_ERR("Invalid handshake response: no secret");
                cb2(nullptr);
                return;
            }
            const auto secret_part2 = body.substr(start, eol - start);
            std::string sig = body.substr(eol + 1);
            if (!Crypto::SSL::verify(key, bearer_token_we_use + '\n' + secret_part2, sig)) {
                cb2(nullptr);
                DEBUG_LOG("Handshake response has invalid signature");
                return;
            }

            secret += secret_part2;
            // Make Peer
            auto p = std::make_shared<Peer>(
                domain,
                PeerAuth(
                    secret,
                    bearer_token_we_use,
                    std::move(token)
                )
            );

            // Add peer
            m_mtx.write_lock();
            m_peers_in[p->m_auth.m_bearer_token_we_accept] = p; // was nullptr
            auto ret = m_peers_out.emplace(domain, p);
            if (!ret.second) {
                DEBUG_LOG("Duplicate peer?? - " <<domain);
                m_peers_in.erase(p->m_auth.m_bearer_token_we_accept);
                m_mtx.write_unlock();
                cb2(nullptr);
                return;
            }
            m_mtx.write_unlock();

            // Success
            cb2(p);
            DEBUG_LOG("New peer: " <<domain);
        };

        auto handshake_err_cb = [this, domain, cb2 = std::move(cb)] (std::string err) {
            cb2(nullptr);
            LOG_ERR("Peer handshake failed " <<domain <<": " <<err);
            // Safe to leave the peer stub in the cache
        };

        bool use_https = domain.find(':') == std::string::npos;
        if (use_https)
            g_fiy->m_https.request(
                domain,
                std::move(req),
                std::move(handshake_cb),
                std::move(handshake_err_cb)
            );
        else
            g_fiy->m_http.request(
                domain,
                std::move(req),
                std::move(handshake_cb),
                std::move(handshake_err_cb)
            );
    };

    auto err_key_cb = [domain, cb](std::string message) {
        LOG_ERR("Could not get key for peer " <<domain <<": " <<message);
        cb(nullptr);
    };

    boost::beast::http::request<boost::beast::http::empty_body> key_req;
    key_req.target("/peer/key");
    key_req.method(boost::beast::http::verb::get);
    key_req.keep_alive(false);

    if (domain.find(':') == std::string::npos)
        g_fiy->m_https.request(
            domain,
            std::move(key_req),
            std::move(with_key_cb),
            std::move(err_key_cb)
        );
    else
        g_fiy->m_http.request(
            domain,
            std::move(key_req),
            std::move(with_key_cb),
            std::move(err_key_cb)
        );
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
        m_body = req->body == nullptr
            ? ""
            : std::string(req->body, req->body_len);
        m_method = req->method;
    }

    [[nodiscard]] fiy_request_t temp_copy() const {
        return fiy_request_t{
            .domain=m_domain.empty() ? nullptr : m_domain.c_str() ,
            .user=m_user.empty() ? nullptr : m_user.c_str(),
            .method=m_method,
            .path=m_path.empty() ? nullptr : m_path.c_str(),
            .headers=m_headers.empty() ? nullptr : m_headers.c_str(),
            .body_len=m_body.size(),
            .body=m_body.empty() ? nullptr : m_body.data(),
        };
    }
};

void Peers::request_peer(
    const std::string& domain,
    const std::string& appid,
    const fiy_request_t* req,
    void* context,
    void (*callback)(const fiy_response_t*, void*)
) {
    if (domain.empty() || domain == g_fiy->m_config.m_hostname) {
        DEBUG_LOG("request_peer(localhost)");
        g_fiy->m_mods.get_mod(appid)->m_ipc->handle_request(req, context, callback);
    }

    auto p = get_peer_for_domain(domain);
    if (p == nullptr) {
        DEBUG_LOG("Peer " <<domain <<" not in cache");
        new_peer(
            domain,
            [appid, request = ScopedRequest(req), callback, context]
            (const std::shared_ptr<Peer>& p) {
                if (p != nullptr) {
                    DEBUG_LOG("sending request to peer " <<p->m_domain );
                    auto r = request.temp_copy(); // copied earlier to prevent invalidation
                    request_peer(p, appid, &r, context, callback);
                } else {
                    DEBUG_LOG("couldn't link with peer");
                    if (callback != nullptr)
                        callback(nullptr, context);
                }
            }
        );
        return;
    } else {
        request_peer(p, appid, req, context, callback);
    }
}

void Peers::request_peer(
    const std::shared_ptr<Peer>& peer,
    const std::string& appid,
    const fiy_request_t* req,
    void* context,
    void (*callback)(const fiy_response_t*, void*)
) {
    // Null check
    if (peer == nullptr) {
        if (callback != nullptr)
            callback(nullptr, context);
        return;
    }

    std::string user = req->user != nullptr ? req->user : "";
    std::string path = req->path != nullptr ? req->path : "";

    boost::beast::http::request<boost::beast::http::string_body> request;
    request.method((boost::beast::http::verb)req->method);
    request.target("/mods/" + appid);
    request.body() = std::string(req->body, req->body_len); // note this should work even if body contains null chars
    request.set("Fiy-Peer", peer->m_auth.m_bearer_token_we_send);
    request.set("Fiy-User", user);
    request.set("Fiy-Path", path);
    std::string now_str = std::to_string(g_fiy->now());
    request.set("Fiy-Now", now_str);
    request.set("Authorization", "FIY1 " + peer->sig(appid, path, user, req->body_len, now_str));
    request.set(boost::beast::http::field::host, req->domain);
    response_set_headers(request, req->headers);
    request.prepare_payload();

    auto cb = [context, callback] (boost::beast::http::response<boost::beast::http::string_body> res) {
//        std::cout <<"p2p response: " <<res <<std::endl;
        if (callback == nullptr)
            return;
        auto headers_str = get_headers_string(res);
        const fiy_response_t response{
            .status = static_cast<int>(res.result_int()),
            .headers = headers_str.c_str(),
            .body_len = res.body().size(),
            .body = res.body().data(),
        };
        callback(&response, context);
    };

    auto err_cb = [context, callback](std::string err) {
        if (callback == nullptr)
            return;
        const fiy_response_t response{
            .status = -1,
            .headers = "",
            .body_len = err.size(),
            .body = err.c_str(),
        };
        callback(&response, context);
        LOG_ERR("Peer request failed: " << err);
    };

    bool use_https = std::string_view(peer->m_domain).find(':') == std::string_view::npos;
    if (use_https) {
        g_fiy->m_https.request(peer->m_domain, std::move(request), std::move(cb), std::move(err_cb));
    } else {
        g_fiy->m_http.request(peer->m_domain, std::move(request), std::move(cb), std::move(err_cb));
    }
}