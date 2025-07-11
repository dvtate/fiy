//
// Created by tate on 6/20/24.
//

#pragma once

#include "drogon/HttpController.h"


class PortalRoutes : public drogon::HttpController<PortalRoutes, false> {
public:
    PortalRoutes() = default;

    static void login_get(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    static void login_post(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    static void signup_get(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    static void signup_post(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    static void home_get(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    static void main_js(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
//    static void main_css(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
//    static void settings_get(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
//    static void settings_post(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    static void test(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);

//    METHOD_LIST_BEGIN
//        ADD_METHOD_TO(PortalRoutes::login_get, "/portal/login", drogon::HttpMethod::Get, "ModSdCheckMiddleware");
//        ADD_METHOD_TO(PortalRoutes::login_post, "/portal/login", drogon::HttpMethod::Post, "ModSdCheckMiddleware");
//        ADD_METHOD_TO(PortalRoutes::signup_get, "/portal/signup", drogon::HttpMethod::Get, "ModSdCheckMiddleware");
//        ADD_METHOD_TO(PortalRoutes::signup_post, "/portal/signup", drogon::HttpMethod::Post, "ModSdCheckMiddleware"); // TODO this should be via invite code only
//        ADD_METHOD_TO(PortalRoutes::main_js, "/portal/main.js", drogon::HttpMethod::Get, "ModSdCheckMiddleware");
//        ADD_METHOD_TO(PortalRoutes::home_get, "/portal", drogon::HttpMethod::Get, "ModSdCheckMiddleware");
//        ADD_METHOD_TO(PortalRoutes::test, "/test", drogon::HttpMethod::Get, "ModSdCheckMiddleware");
////        ADD_METHOD_TO(PortalRoutes::settings_get, "/portal/settings", drogon::HttpMethod::Get, "ModSdCheckMiddleware");
////        ADD_METHOD_TO(PortalRoutes::settings_post, "/portal/settings", drogon::HttpMethod::Post, "ModSdCheckMiddleware");
////        ADD_METHOD_TO(PortalRoutes::main_css, "/portal/main.css", drogon::Get, "ModSdCheckMiddleware");
//    METHOD_LIST_END
};
