//
// Created by tate on 10/28/25.
//

#pragma once

#include <functional>
#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>

#include "fediymod.hpp"

namespace fiy {
    // TODO replace this with a more performant trie-based implementation
    // TODO middleware?
    class Router {
    public:
        using Handler = std::function<void(Request&, fiy_callback_t, std::vector<std::string_view>&&)>;
        using NotFoundHandler = std::function<void(Request&, fiy_callback_t)>;

        Router& get(const std::string& path, Handler handler) {
            return add_route(Request::Method::GET, path, std::move(handler));
        }

        Router& add_route(const Request::Method method, const std::string& path, Handler handler) {
            m_routes[method].emplace_back(parse_route(path, std::move(handler)));
            return *this;
        }

        Router& not_found(NotFoundHandler handler) {
            m_not_found_handler = std::move(handler);
            return *this;
        }

        void handle_request(Request& request, fiy_callback_t cb) {
            auto it = m_routes.find(static_cast<Request::Method>(request.method));
            if (it != m_routes.end()) {
                for (const auto& route : it->second) {
                    auto params = match(request.path, route);
                    if (params.has_value()) {
                        route.handler(request, cb, std::move(*params));
                        return;
                    }
                }
            }
            if (m_not_found_handler)
                m_not_found_handler.value()(request, cb);
        }

    private:
        struct Route {
            std::vector<std::string> parts; // route parts split by '/' , "" when param
            Handler handler;
        };

        std::unordered_map<Request::Method, std::vector<Route>> m_routes;
        std::optional<NotFoundHandler> m_not_found_handler;

        static Route parse_route(const std::string& path, Handler handler) {
            Route route;

            std::size_t i = 1;
            std::size_t slash;
            while ((slash = path.find('/', i)) != std::string::npos) {
                std::string part = path.substr(i, slash - i);
                if (part == "{}")
                    route.parts.emplace_back("");
                else if (part == "\\{}")
                    route.parts.emplace_back("{}");
                else
                    route.parts.emplace_back(std::move(part));

                i = slash;
            }
            route.handler = std::move(handler);
            return route;
        }

        static std::optional<std::vector<std::string_view>> match(const std::string& path, const Route& route) {
            std::vector<std::string_view> result;

            size_t start = 0, end = 0;
            size_t i = 0;
            while (end != std::string::npos) {
                end = path.find('/', start);
                std::string_view segment = (end == std::string::npos)
                    ? std::string_view(path).substr(start)
                    : std::string_view(path).substr(start, end - start);

                if (segment.empty()) {
                    start = end + 1;
                    continue;
                }

                if (i >= route.parts.size())
                    return std::nullopt;

                // Either param or segment
                if (route.parts[i].empty())
                    result.push_back(segment);
                else
                    if (segment != route.parts[i])
                        return std::nullopt;

                ++i;
                start = end + 1;
            }

            if (i != route.parts.size())
                return std::nullopt;

            return result;
        }
    };
}