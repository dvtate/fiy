//
// Created by tate on 7/15/25.
//

#ifndef FEDIY_FEDIYMOD_HPP
#define FEDIY_FEDIYMOD_HPP

#include <string>
#include <string_view>
#include <cstring>
#include <functional>
#include <optional>
#include <unordered_map>

#include "fediymod.h"

namespace fiy {

    struct HostInfo : public fiy_host_info_t {

        // TODO
        // static HostInfo* singleton;

        HostInfo(fiy_host_info_t hi): fiy_host_info_t(hi) {}
        HostInfo() = default;

        [[nodiscard]] std::string host_base_uri() const {
            static const std::string protocol =
                base_uri[4] == 's' ? "https://" : "http://";
            return protocol + domain;
        }

        /**
         * Write a log
         * @param type log level
         *  - 0 : fatal
         *  - 1 : error
         *  - 2 : warning
         *  - 3 : info
         *  - 4 : debug
         * @param msg log message text
         */
        void log(const int type, const std::string& msg) const {
            fiy_host_info_t::log(type, msg.c_str());
        }

        enum class Log : int {
            FATAL = 0,
            ERROR = 1,
            WARNING = 2,
            INFO = 3,
            DEBUG = 4,
        };

        /**
         * Write a log
         * @param type log type/severity
         * @param msg log message text
         */
        void log(const Log type, const std::string& msg) const {
            log(static_cast<int>(type), msg);
        }
        void log_fatal(const std::string& msg) const {
            log(Log::FATAL, msg);
        }
        void log_error(const std::string& msg) const {
            log(Log::ERROR, msg);
        }
        void log_warning(const std::string& msg) const {
            log(Log::WARNING, msg);
        }
        void log_info(const std::string& msg) const {
            log(Log::INFO, msg);
        }
        void log_debug(const std::string& msg) const {
            log(Log::DEBUG, msg);
        }

        /**
         * Get components from user string
         * @param user user string of format user@domain
         * @return pair < user, domain > of same format
         */
        std::pair<std::string_view, std::string_view> split_user_str(std::string_view user) {
            const auto at_idx = user.find('@');

            // No user
            if (user.empty())
                return std::make_pair("","");

            // Local user
            if (at_idx >= user.size() - 1 // includes "user@" and "user"
                || user.substr(at_idx + 1) == this->domain
            )
                return std::make_pair(user.substr(0, at_idx), "");

            // Remote user
            return std::make_pair(
                user.substr(0, at_idx),
                user.substr(at_idx + 1)
            );
        }
    };

    struct Response : public fiy_response_t {
        explicit Response(
            int status,
            const char* body,
            std::size_t body_len,
            std::string headers_str = ""
        ):
            m_headers(std::move(headers_str))
        {
            this->status = status;
            this->body = body;
            this->body_len = body_len;
            headers = m_headers.c_str();
        }

    protected:
        std::string m_headers;

        void add_header(const std::string_view field, const std::string_view value) {
            m_headers += field;
            m_headers += ": ";
            m_headers += value;
            m_headers += '\n';
            headers = m_headers.c_str();
        }
        void add_header(const std::string_view header) {
            m_headers += header;
            m_headers += '\n';
            headers = m_headers.c_str();
        }

        void clear_headers() {
            m_headers.clear();
            headers = nullptr;
        }

        void set_headers(const std::string_view headers_str) {
            m_headers = headers_str;
            headers = m_headers.c_str();
        }

#if 0
        ~Response() {
            clear_headers();
        }
#endif
    };

    struct Request : public fiy_request_t {

        enum Method : uint8_t {
            UNKNOWN = 0,
            DELETE, GET, HEAD, POST, PUT, CONNECT, OPTIONS,
            TRACE, COPY, LOCK, MKCOL, MOVE, PROPFIND, PROPPATCH,
            SEARCH, UNLOCK, BIND, REBIND, UNBIND,
            ACL, REPORT, MKACTIVITY, CHECKOUT, MERGE, MSEARCH,
            NOTIFY, SUBSCRIBE, UNSUBSCRIBE, PATCH, PURGE,
            MKCALENDAR, LINK, UNLINK
        };

        explicit Request(
            const char* path,
            uint8_t method = (uint8_t) Method::GET,
            const char* domain = nullptr,
            const char* user = nullptr,
            const char* headers = nullptr,
            const char* body = nullptr,
            const size_t body_len = 0
        ): fiy_request_t{
            .domain=domain,
            .user=user,
            .path=path,
            .headers=headers,
            .body=body,
            .body_len=body_len,
            .method=method
        } {
        }

        Request(fiy_request_t req): fiy_request_t(req) {}

        [[nodiscard]] std::string user_str(const std::string& anon_name = "anon") const {
            if (user == nullptr)
                return anon_name;
            if (domain == nullptr)
                return user;
            return std::string(user) + "@" + std::string(domain);
        }
        [[nodiscard]] const char* method_str() const {
            return fiy_http_verb_strings[method];
        }
        [[nodiscard]] bool is_local() const {
            return user != nullptr && domain == nullptr;
        }

        inline void respond(fiy_callback_t cb, const fiy_response_t& resp) {
            cb(this, &resp);
        }

        /**
         * Respond to request
         * @param cb provided callback function
         * @param status HTTP status code
         * @param body HTTP body
         * @param headers null-terminated headers string
         */
        inline void respond(fiy_callback_t cb, int status = 200, const std::string_view body = "", const std::string_view headers = "") {
            fiy_response_t res = {
                .status=status,
                .body=body.empty() ? nullptr : body.data(),
                .body_len=body.size(),
                .headers=headers.empty() ? nullptr : headers.data()
            };
            cb(this, &res);
        }
        inline void respond(fiy_callback_t cb, const std::string_view body, const std::string_view headers = "") {
            respond(cb, 200, body, headers);
        }

        enum class Locality {
            USER = 0,       // Request is from the user
            INSTANCE = 1,   // Request is from another user on same instance
            FEDIVERSE = 2,  // Request is from another user in the fediverse
            PUBLIC = 3      // Request is anonymous
        };

        [[nodiscard]] Locality locality(const std::string& local_user) const {

            return user == nullptr
                ? Locality::PUBLIC
                : domain == nullptr
                    ? (local_user != user ? Locality::INSTANCE : Locality::USER)
                    : Locality::FEDIVERSE;

            // TODO maybe a use case for local app-to-app requests
            //      in this case the user would be null and domain would be host domain
            //      this would probably mean privilege level -1
        }
    };

    // TODO replace this with a more performant trie-based implementation
    // TODO middleware?
    // TODO move this to separate file
    class Router {
    public:
        using Handler = std::function<void(Request&, fiy_callback_t, std::vector<std::string_view>&&)>;
        using NotFoundHandler = std::function<void(Request&, fiy_callback_t)>;

        Router& get(const std::string& path, Handler handler) {
            return add_route(Request::Method::GET, path, std::move(handler));
        }

        Router& add_route(Request::Method method, const std::string& path, Handler handler) {
            Route route = parse_route(path, std::move(handler));
            m_routes[method].push_back(std::move(route));
            return *this;
        }

        Router& not_found(NotFoundHandler handler) {
            m_not_found_handler = std::move(handler);
            return *this;
        }

        void handle_request(Request& request, fiy_callback_t cb) {
            auto it = m_routes.find((Request::Method) request.method);
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
            std::vector<std::string> parts; // route parts split by '/'
            std::vector<bool> is_param;     // which parts are parameters
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
                route.is_param.push_back(part == "{}");
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

                if (route.is_param[i]) {
                    result.push_back(segment);
                } else {
                    if (segment != route.parts[i])
                        return std::nullopt;
                }

                ++i;
                start = end + 1;
            }

            if (i != route.parts.size())
                return std::nullopt;

            return result;
        }
    };

} // namespace fiy

#endif //FEDIY_FEDIYMOD_HPP
