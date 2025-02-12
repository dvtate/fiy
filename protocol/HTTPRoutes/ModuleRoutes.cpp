//
// Created by tate on 6/25/24.
//

#include "ModuleRoutes.hpp"

#include "LocalUser.hpp"
#include "App.hpp"

ModuleRoutes::User ModuleRoutes::find_user(const drogon::HttpRequestPtr& req) {
    static const User unauthenticated = { .domain=nullptr, .user=" "};

    // Local User authentication
    auto u = req->session()
        ->getOptional<std::shared_ptr<LocalUser>>("user").value_or(nullptr);
    if (u != nullptr)
        return { .domain=nullptr, .user=u->get_username() };

    // Peer authentication
    // TODO
    auto peer_token = req->getHeader("Fediy-Peer");
    if (peer_token.empty()) {
        std::cout <<"missing peer auth token\n";
        return unauthenticated;
    }
    auto user = req->getHeader("Fediy-User");
    auto p = g_app->m_peers.get_peer_from_token(peer_token);
    if (p != nullptr) {
        std::cout <<"remote user authenticated\n";
        return { .domain=p->m_domain, .user=user };
    } else {
        // Failed auth equals unauthenticated
        DEBUG_LOG("invalid peer auth token: " <<peer_token);
        return unauthenticated;
    }
}

void ModuleRoutes::app_redirect(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    const auto m = g_app->m_mods.get_mod_by_id(req->getRoutingParameters()[0]);
    if (m == nullptr) {
        static auto ret404 = drogon::HttpResponse::newNotFoundResponse();
        callback(ret404);
    } else {
//        std::cout <<"redirecting to /" <<m->m_path <<std::endl;
        callback(drogon::HttpResponse::newRedirectionResponse("/" + m->m_path));
    }
}

void ModuleRoutes::app_send_msg(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    // Check to see if it's using a subdomain
    std::string app, uri;
    auto hostname = req->getHeader("Host");
    if (!hostname.empty()) {
        std::string_view hhn = g_app->m_config.m_hostname;
        if (hostname.ends_with(hhn) && hhn.size() != hostname.size()) {
            // Subdomain app  app.example.com/uri/path
            app = hostname.substr(0, hostname.size() - hhn.size() - 1);
            uri = req->path();
        } else if (hostname != hhn) {
            // Invalid request to different host?
            auto r = drogon::HttpResponse::newHttpResponse(
                drogon::HttpStatusCode::k400BadRequest,
                drogon::ContentType::CT_TEXT_HTML
            );
            r->setBody(std::string("Wrong host? Expected host to be ") + hhn.data() + " but host was " + hostname);
            callback(r);
            return;
        } else {
            // Not a subdomain app  example.com/app/uri/path
            const auto slash_idx = req->path().find('/', 1);
            std::cout <<"slash_idx = " <<slash_idx <<std::endl;
            if (slash_idx == -1) {
                app = req->path().data() + 1;
                uri = "/";
            } else {
                app = req->path().substr(1, slash_idx - 1);
                uri = req->path().data() + slash_idx;
            }
        }
    }

    // Forward to app
    DEBUG_LOG("Calling " <<app <<" : " << uri);
    Mod* m = g_app->m_mods.get_mod_by_path(app);
    if (m == nullptr) {
        callback(drogon::HttpResponse::newNotFoundResponse(req));
        return;
    }
    req->setPath(uri);
    m->m_ipc->handle_request(req, find_user(req), std::move(callback));
}

void ModuleRoutes::app_remote_msg(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) {
    Mod* m = g_app->m_mods.get_mod_by_id(req->getRoutingParameters()[0]);
    if (m == nullptr) {
        DEBUG_LOG("no mod: " <<req->getRoutingParameters()[0]);
        callback(drogon::HttpResponse::newNotFoundResponse(req));
        return;
    }
    req->setPath(req->getHeader("Fediy-Path"));
    auto user = find_user(req);
    if (user.domain == nullptr && !req->getHeader("Fediy-Peer").empty()) {
        // TODO tell the peer to reauthenticate
    }
    m->m_ipc->handle_request(req, std::move(user), std::move(callback));
}

void ModSdCheckMiddleware::invoke(
    const drogon::HttpRequestPtr& req,
    drogon::MiddlewareNextCallback&& nextCb,
    drogon::MiddlewareCallback&& mcb
) {
    // It's an app!
    static const auto& hn = g_app->m_config.m_hostname;
    if (req->getHeader("Host") != hn) {
        ModuleRoutes::app_send_msg(req, std::move(mcb));
        return;
    }

    // Go next
    nextCb(std::move(mcb));
}