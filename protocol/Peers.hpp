#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <algorithm>
#include <variant>
#include <thread>
#include <functional>

#include "globals.hpp"
#include "../util/RWMutex.hpp"

#include "Peer.hpp"
#include "LocalUser.hpp"

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

    static void request_peer(
        const std::shared_ptr<Peer>& peer,
        const std::string& appid,
        const std::string& user,
        const fiy_request_t* req,
        void* context,
        void (*callback)(const fiy_response_t*, void*)
    );
    void request_peer(
        const std::string& domain,
        const std::string& appid,
        const std::string& user,
        const fiy_request_t* req,
        void* context,
        void (*callback)(const fiy_response_t*, void*)
    );

//    void request_peer(std::shared_ptr<Peer> peer, const std::string& local_user, T& req, callback

};
