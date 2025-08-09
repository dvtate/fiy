//
// Created by tate on 7/15/25.
//

#ifndef FEDIY_FEDIYMOD_HPP
#define FEDIY_FEDIYMOD_HPP

#include <string>
#include <string_view>
#include <cstring>

#include "fediymod.h"

namespace fiy {

    struct Response : public fiy_response_t {
        explicit Response(
            int status,
            const char* body,
            std::size_t body_len,
            std::string headers_str = ""
        ):
            fiy_response_t{ .status=status, .body = body, .body_len = body_len },
            m_headers(std::move(headers_str))
        {
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
    };

    // TODO Method + path Router

} // namespace fiy

#endif //FEDIY_FEDIYMOD_HPP
