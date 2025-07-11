//
// Created by tate on 6/25/24.
//

#pragma once


#include "drogon/HttpController.h"
#include "drogon/HttpMiddleware.h"

#include "drogon/utils/Utilities.h"

class ModuleRoutes : public drogon::HttpController<ModuleRoutes, false> {
public:
    ModuleRoutes() = default;

    struct User {
        const char* domain{nullptr};
        std::string user;
    };

    static User find_user(const drogon::HttpRequestPtr& req);

    static void app_redirect(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    static void app_send_msg(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    static void app_remote_msg(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

//    METHOD_LIST_BEGIN
//        ADD_METHOD_TO(ModuleRoutes::app_redirect, "/mods/{mod_id}", drogon::Get, "ModSdCheckMiddleware");
//        ADD_METHOD_TO(ModuleRoutes::app_remote_msg, "/mods/{mod_id}/msg", "ModSdCheckMiddleware");
//        ADD_METHOD_VIA_REGEX(ModuleRoutes::app_send_msg, ".*");
//    METHOD_LIST_END
};

// Check for subdomains since they can be used for apps
// Unfortunate result of being unable to replace the drogon router
class ModSdCheckMiddleware : public drogon::HttpMiddleware<ModSdCheckMiddleware>
{
public:
    ModSdCheckMiddleware(){}  // do not omit constructor

    void invoke(const drogon::HttpRequestPtr &req,
                drogon::MiddlewareNextCallback &&nextCb,
                drogon::MiddlewareCallback &&mcb) override;
};