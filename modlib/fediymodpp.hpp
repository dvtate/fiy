//
// Created by tate on 7/15/25.
//

#ifndef FEDIY_FEDIYMODPP_HPP
#define FEDIY_FEDIYMODPP_HPP

#include "fediymod.h"

namespace fiy {

    struct Response : public fiy_response_t {
        explicit Response(int status = 200, const char* body = nullptr, const char* headers = nullptr):
                fiy_response_t{ .status=status, .body = body, .headers = headers }
        {}
    };

    struct Request : public fiy_request_t {

        enum class Method : uint8_t {
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
            const char* body = nullptr
        ): fiy_request_t{
            .domain=domain,
            .user=user,
            .path=path,
            .headers=headers,
            .body=body,
            .method=method,
        } {
        }

        [[nodiscard]] std::string user_str(const std::string& anon_name = "anon") const {
            if (user == nullptr)
                return "anon";
            if (domain == nullptr)
                return user;
            return std::string(user) + "@" + std::string(domain);
        }
        [[nodiscard]] const char* method_str() const {
            return fiy_http_verb_strings[method];
        }
        [[nodiscard]] bool is_local() const {
            return domain == nullptr;
        }

        inline void respond(fiy_callback_t cb, const Response& resp) {
            cb(this, &resp);
        }
    };


    struct ModInfo : public fiy_mod_info_t {};
//    struct Request : public fiy_request_t {};
//    struct HostInfo : public fiy_host_info_t {};
//    struct Response : public fiy_response_t {
//        std::string body;
//        std::string headers;
//        int status;
//        explicit Response(std::string body, int status = 200):
//            body(std::move(body)),
//            status(status)
//        {}
//
//        ~Response() {}
//
//        operator struct fiy_response_t() {
//            return (struct fiy_response_t){
//                .status=status,
//                .body=body.c_str(),
//                .headers=headers.c_str()
//            };
//        }
//    };
}

#endif //FEDIY_FEDIYMODPP_HPP
