//
// Created by tate on 7/15/25.
//

#ifndef FEDIY_FEDIYMOD_HPP
#define FEDIY_FEDIYMOD_HPP

#include <string>
#include <string_view>
#include <cstring>
#include <iostream>
#include <map>

#include <unistd.h>

#include "fediymod.h"

namespace fiy {
    using Callback = fiy_callback_t;
    using ModInfo = fiy_mod_info_t;
    using BodyType = fiy_body_type;
    using StartFunction = fiy_mod_start_function_t;

    struct HostInfo : public fiy_host_info_t {
        // TODO
        // static HostInfo* singleton;

        HostInfo() = default;
        HostInfo(fiy_host_info_t hi): fiy_host_info_t(hi) {}

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
        [[nodiscard]] std::pair<std::string_view, std::string_view>
        split_user_str(const std::string_view user) const {
            // No user
            if (user.empty())
                return std::make_pair("","");

            // Local user
            const auto at_idx = user.find('@');
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

    struct Body : public fiy_body_t {
        using Type = fiy_body_type;
        Body(): fiy_body_t({
            .type = FIY_BODY_NONE,
            .buffer = { nullptr, 0 }
        }) {}
        explicit Body(FILE* f): fiy_body_t({
            .type = Type::FIY_BODY_FILE,
            .file = { .fd= fileno(f), .offset = ftell(f) }
        }) {}
        explicit Body(const int file_descriptor, const long offset = 0):
            fiy_body_t({
                .type = Type::FIY_BODY_FILE,
                .file = { .fd = file_descriptor, .offset = offset }
            })
        {}
        explicit Body(const std::string_view body): fiy_body_t({
            .type = body.empty() ? FIY_BODY_NONE : FIY_BODY_BUFFER,
            .buffer = { body.data(), body.length() }
        }) {}
        explicit Body(const char* buffer): fiy_body_t({
            .type = FIY_BODY_BUFFER,
            .buffer = { .data = buffer, .length = strlen(buffer) }
        }) {}
        Body(const char* buffer, const size_t length): fiy_body_t({
            .type = FIY_BODY_BUFFER,
            .buffer = { .data = buffer, .length = length }
        }) {}
        Body(fiy_buffered_reader_fn fn, void* context): fiy_body_t({
            .type = FIY_BODY_READER,
            .reader = { .read = fn, .context = context }})
        {}

        [[nodiscard]] static std::string to_string(const fiy_body_t& body) {
            switch (body.type) {
                case FIY_BODY_NONE:
                    return "";

                case FIY_BODY_BUFFER:
                    return std::string(body.buffer.data, body.buffer.length);

                case FIY_BODY_READER: {
                    std::string ret;
                    char buff[8192];
                    for (;;) {
                        const auto n = body.reader.read(body.reader.context, buff, 8192);
                        if (n > 0)
                            ret.append(buff, n);
                        else
                            return ret;
                    }
                }

                case FIY_BODY_FILE: {
                    std::string ret;
                    char buff[8192];
                    for (;;) {
                        const auto n = read(body.file.fd, buff, sizeof buff);
                        if (n > 0)
                            ret.append(buff, n);
                        else
                            return ret;
                    }
                }

                // invalid
                default:
                    std::cerr <<"fiy::Body::to_string(): Invalid body type!!\n";
                    // std::unreachable();
                    return "";
            }
        }

        /**
         * Read contents of body into string
         * @return body as a string
         */
        [[nodiscard]] std::string to_string() const {
            return to_string(*this);
        }
    };

    struct Response : public fiy_response_t {
    protected:
        std::string m_headers;

    public:
        Response():
            fiy_response_t({
                .status = 200,
                .headers = nullptr,
                .body = Body()
            })
        {}

        explicit Response(
            const int status,
            std::string headers_str = "",
            const Body& body = Body()
        ):
            m_headers(std::move(headers_str))
        {
            this->status = status;
            this->headers = m_headers.c_str();
            this->body = body;
        }

        Response(const Response& other) {
            this->status = other.status;
            this->body = other.body;
            m_headers = other.m_headers;
            headers = m_headers.c_str();
        }

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

    /// Degrees of separation
    enum class Locality {
        USER = 0,       /// Request is from the user
        INSTANCE = 1,   /// Request is from another user on same instance
        FEDIVERSE = 2,  /// Request is from another user in the fediverse
        PUBLIC = 3      /// Request is anonymous
    };

    // Must not add members or else not pointer compatible
    struct Request : public fiy_request_t {

        // match those from boost::beast::http::verb
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
            const uint8_t method = (uint8_t) Method::GET,
            const char* domain = nullptr,
            const char* user = nullptr,
            const char* headers = nullptr,
            const char* body = nullptr,
            const size_t body_len = 0
        ): fiy_request_t{
            .domain=domain,
            .user=user,
            .method=method,
            .path=path,
            .headers=headers,
            .body_len=body_len,
            .body=body,
        } {
        }

        Request(const fiy_request_t& req): fiy_request_t(req) {}

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
        [[nodiscard]] static const char* method_str(const uint8_t method) {
            return fiy_http_verb_strings[method];
        }
        [[nodiscard]] bool is_local() const {
            return user != nullptr && domain == nullptr;
        }

        inline void respond(const fiy_callback_t cb, const fiy_response_t& resp) const {
            cb(this, &resp);
        }

        /**
         * Respond to request
         * @param cb provided callback function
         * @param status HTTP status code
         * @param body HTTP body
         * @param headers null-terminated headers string
         */
        // void respond(
        //     const fiy_callback_t cb,
        //     const int status = 200,
        //     const std::string_view body = "",
        //     const std::string& headers = ""
        // ) const {
        //     const fiy_response_t res = {
        //         .status=status,
        //         .headers=headers.empty() ? nullptr : headers.c_str(),
        //         .body = body.empty() ? Body() : Body(body),
        //     };
        //     cb(this, &res);
        // }
        void respond(
            const fiy_callback_t cb,
            const int status = 200,
            const std::string& headers = "",
            const Body& body = Body()
        ) const {
            const fiy_response_t res = {
                .status = status,
                .headers = headers.empty() ? nullptr : headers.c_str(),
                .body = body,
            };
            cb(this, &res);
        }

        /**
         * Get the trust level of a given request wrt. a local user
         * @param local_user local user this request wants information on
         * @return Locality trust level
         */
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

        /**
         * Get the first value for a give http header
         * @param key HTTP header key to find
         * @return First value associated with given header
         */
        // FIXME
        [[nodiscard]] std::string_view find_header(const std::string_view key) const {
            if (this->headers == nullptr)
                return "";

            std::string_view fields = this->headers;
            std::size_t start = 0;
            do {
                // Find colon
                std::size_t end = fields.find(':', start);
                if (end == std::string_view::npos)
                    return "";

                // Check if matching key
                if (end == key.size()) {
                    for (size_t i = 0; i < end - start; i++)
                        if (tolower(fields[start + i]) != tolower(key[i]))
                            goto next_field;

                    // Return corresponding value
                    std::size_t endl = fields.find('\n', end);
                    if (endl == std::string_view::npos)
                        endl = fields.size();
                    return fields.substr(end + 1, endl - end - 1);
                }

                // Check next field
next_field:
                start = fields.find('\n', end);
            } while (start != std::string_view::npos);

            return "";
        }

        /**
         * Parse the headers into a map where all the keys are lower-case
         * @return headers lookup map
         */
        [[nodiscard]] std::map<std::string, std::string_view> headers_map() const {
            std::map<std::string, std::string_view> ret;

            if (this->headers == nullptr)
                return ret;

            std::string_view fields = this->headers;
            std::size_t start = 0;
            std::string key;
            key.reserve(32);
            do {
                // Find colon
                std::size_t end = fields.find(':', start);
                if (end == std::string_view::npos)
                    return ret;

                // Convert key to lower case
                key.clear();
                for (size_t i = 0; i < end - start; i++)
                    key += static_cast<char>(tolower(fields[start + i]));

                // Find next start
                start = fields.find('\n', end);
                if (start == std::string_view::npos)
                    start = fields.size();

                // Put it into the map
                end++;
                if (fields[end] == ' ')
                    end++;
                auto v = fields.substr(end, start - end);
                if (v.back() == '\r')
                    v.remove_suffix(1);
                ret.emplace(key, v);

                // Check next field
                start++; // after \n
            } while (start != std::string_view::npos);

            return ret;
        }
    };

} // namespace fiy

using fiy::fiy_request_t;
using fiy::fiy_response_t;
using fiy::fiy_host_info_t;

#endif //FEDIY_FEDIYMOD_HPP
