#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>

#include "../util/RWMutex.hpp"
#include "globals.hpp"

#include "LocalUser.hpp"
#include "Peer.hpp"

#include "../modlib/fediymod.h"

class Peers {
    RWMutex m_mtx;

    // domain -> peer
    std::unordered_map<std::string, std::shared_ptr<Peer>> m_peers_out;

    // bearer token -> peer
    std::unordered_map<std::string, std::shared_ptr<Peer>> m_peers_in;

    std::thread m_cron;

public:
    Peers();

    //    void load_peers_from_db();

    void prune();

    bool add_peer(const std::string& domain, const std::shared_ptr<Peer>& p);

    std::shared_ptr<Peer> get_peer_for_domain(const std::string& domain);
    std::shared_ptr<Peer> get_peer_from_token(const std::string& domain);

    void new_peer(const std::string& domain, std::function<void(const std::shared_ptr<Peer>&)> cb);

    static void request_peer(const std::shared_ptr<Peer>& peer, const std::string& appid,
                             const fiy_request_t* req, void* context,
                             void (*callback)(const fiy_response_t*, void*));
    void request_peer(const std::string& domain, const std::string& appid, const fiy_request_t* req,
                      void* context, void (*callback)(const fiy_response_t*, void*));

    //    void request_peer(std::shared_ptr<Peer> peer, const std::string& local_user, T& req,
    //    callback
};
